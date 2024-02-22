#include <regex>
#include <numeric>
#include <filesystem>
#include <algorithm>
#include <regex>

#include <itertools/itertools.hpp>
#include <clang/AST/QualTypeNames.h>
//#include <clang/AST/DeclFriend.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "data.hpp"
#include "utility/macros.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include "clu/misc.hpp"
#include "clu/concept.hpp"

#include "./worker.hpp"
static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
} logs;

worker_t::worker_t(clang::CompilerInstance *ci) : ci{ci} {
  auto p                  = std::filesystem::path{ci->getFrontendOpts().Inputs[0].getFile().str()};
  module_info.sourcefile  = str_t{p.filename()};
  module_info.module_name = str_t{p.stem()};
}

//--------------------------------------------------------

// Analyze the add_methods_to<A> fields and extract the dispatch in it
void worker_t::get_additional_methods() {

  // We first build a reverse table ptr -> info for the classes
  // Meanwhile, for each class, we call force_instantiation_add_methods
  // which ensure that c2py::module_info::add_methods_to is fully instantiated
  // (in case the user has done a partial instantiation)
  std::map<cls_ptr_t, cls_info_t *> ptr_to_info;
  for (auto &[n, cls_info] : this->module_info.classes) {
    auto b = clu::satisfy_concept(cls_info.ptr, this->force_instantiation_add_methods, this->ci);
    EXPECTS(b);
    ptr_to_info.insert({cls_info.ptr, &cls_info});
  }

  for (auto const &spe : this->add_methods_to->specializations()) {
    if (auto const *cls = spe->getTemplateArgs()[0].getAsType()->getAsCXXRecordDecl()) { // pick up the <CLS> arg
      // we search for this cls in the table
      if (auto it = ptr_to_info.find(cls); it != ptr_to_info.end()) {
        for (clang::Decl const *decl : spe->decls())
          if (auto *v = llvm::dyn_cast<clang::VarDecl>(decl)) analyse_dispatch(it->second->methods, v);
      }
    }
  }
}

//--------------------------------------------------------

// Given cls, stores its methods and friend functions
void scan_class_elements(cls_info_t &cls_info, module_info_t &m_info, cls_ptr_t cls) {

  const bool is_base_class = (cls != cls_info.ptr);

  for (clang::Decl *decl : cls->decls()) { // all declarations in the class
    if (decl->getAccess() != clang::AS_public) continue;

    // -------- method
    if (auto *m = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {

      if (clu::has_annotation(m, "c2py_ignore")) continue;
      if (llvm::isa<clang::CXXDestructorDecl>(m)) continue;                 // no destructors
      if (m->isMoveAssignmentOperator()) continue;                          // no move assign
      if (auto *ctr = llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(m); //
          ctr and ctr->isCopyOrMoveConstructor())
        continue; // no move or copy constructor

      if (m_info.reject_names and std::regex_match(m->getNameAsString(), m_info.reject_names.value())) {
        logs.rejected(fmt::format(R"RAW({0} [{1}])RAW", m->getNameAsString(), "reject_names"));
        //clu::emit_warning(m, "Rejected this method");
        continue;
      }

      // Operators : keep only [] and ()
      static auto const re = std::regex{"operator(.*)"};
      std::smatch ma;
      str_t qname = m->getNameAsString();
      if (std::regex_match(qname, ma, re)) {
        static auto ok_ops = std::vector<str_t>{"[]", "()"};
        if (llvm::find(ok_ops, ma[1].str()) == ok_ops.end()) continue;
      }

      if (llvm::isa<clang::CXXConstructorDecl>(m)) {
        if (not is_base_class) cls_info.constructors.push_back({m});
      } else
        cls_info.methods[m->getNameAsString()].push_back({m});
    }

    // -------- fields
    else if (auto *f = llvm::dyn_cast<clang::FieldDecl>(decl)) {
      cls_info.fields.push_back(f);
    }
  }
}

// ------------------------------------------------------

//
void worker_t::scan_class_and_bases_elements() {
  std::vector<cls_ptr_t> merged;
  auto &allcls = this->module_info.classes;
  for (auto &[_, cls_info] : allcls) {
    scan_class_elements(cls_info, this->module_info, cls_info.ptr);

    for (auto b : cls_info.ptr->bases()) {
      if (b.getAccessSpecifier() != clang::AccessSpecifier::AS_public) continue; // only public bases
      auto *c               = b.getType()->getAsCXXRecordDecl();
      bool c_is_not_wrapped = std::find_if(allcls.begin(), allcls.end(), [c](auto &&p) { return p.second.ptr == c; }) == allcls.end();
      if (c_is_not_wrapped) {
        // We merge the element of the base into the class in progress.
        scan_class_elements(cls_info, this->module_info, c);
      } else {
        if (cls_info.base != nullptr) clu::emit_error(cls_info.ptr, "This class has more than one bases to wrap");
        cls_info.base = c;
      }
    }
  }
}

// -----------------------
// Takes a list of methods, and return a list without const/non const method
// Choose the non-const version if there is both.
//
std::vector<fnt_info_t> flist_rm_const(std::vector<fnt_info_t> const &mlist) {

  using qual_type_vec_t = llvm::SmallVector<clang::QualType>;
  std::vector<std::pair<qual_type_vec_t, int>> v(mlist.size());

  auto get_parameters = [](fnt_ptr_t f) -> qual_type_vec_t {
    qual_type_vec_t res;
    res.reserve(f->getNumParams());
    for (auto const &p : f->parameters()) res.push_back(p->getOriginalType());
    //for (auto i : itertools::range(f->getNumParams())) res[i] = f->getParamDecl(i)->getOriginalType();
    return res;
  };

  std::transform(mlist.begin(), mlist.end(), v.begin(), [&get_parameters](auto &&f) {
    auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f.ptr); // can be null
    return std::pair{get_parameters(f.ptr), (m and m->isConst() ? 0 : 1)};
  });

  std::vector<int> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&v](auto i, auto j) { return v[i] < v[j]; });
  auto last = std::unique(idx.begin(), idx.end(), [&v](auto i, auto j) { return v[i].first == v[j].first; });
  idx.erase(last, idx.end());
  std::sort(idx.begin(), idx.end());
  std::vector<fnt_info_t> res;
  res.reserve(idx.size());
  for (int i : idx) res.push_back(mlist[i]);
  return res;
}

// -------------------------------------

// FIXME : move in utility
bool contains(auto const &v, auto const &x) { return std::find(std::begin(v), std::end(v), x) != std::end(v); }

// -------------------------------------

// PROP of the class
void worker_t::prepare_methods() {
  for (auto &[_, cls] : this->module_info.classes) {

    bool has_user_defined_call = cls.methods.contains("__call__");
    cls.has_hdf5               = HasHdf5 and clu::satisfy_concept(cls.ptr, HasHdf5, this->ci);

    // Serialization
    if (clu::satisfy_concept(cls.ptr, this->HasSerializeLikeBoost, this->ci))
      cls.serialization = Serialization::Tuple;
    else if (cls.has_hdf5)
      cls.serialization = Serialization::H5;

    decltype(cls.methods) methods2;

    for (auto &[n, v] : cls.methods) {
      // operator[] is special, we keep the const AND non const method
      // and split them in getitems, setitems
      int nparam                   = int(v[0].ptr->getNumParams());
      static const char *beg_end[] = {"begin", "end", "cbegin", "cend"}; // NOLINT Questionable

      // special cases first
      // [] : we place the methods in get/setitems depending on constness
      if (n == "operator[]") {
        llvm::copy_if(v, std::back_inserter(cls.getitems), [](auto &&fi) {
          auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(fi.ptr);
          return m->isConst();
        });

        llvm::copy_if(v, std::back_inserter(cls.setitems), [](auto &&fi) {
          auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(fi.ptr);
          return not m->isConst();
        });
      } else if (n == "operator()") {
        if (not has_user_defined_call) methods2.insert({"__call__", flist_rm_const(v)});
      }
      // size
      else if ((n == "size") and (nparam == 0))
        cls.has_size_method = true;
      // iterator methods : begin, end and co
      else if (contains(beg_end, n) and (nparam == 0))
        cls.has_iterator = true;
      // all other methods : just remove the const, non const duplicate
      else
        methods2.insert({n, flist_rm_const(v)});
    }
    // the methods are now filtered, const duplicate is removed
    cls.methods = std::move(methods2);
  }
}
// -------------------------------------

void worker_t::remove_multiple_decl() {
  auto &M = this->module_info;
  for (auto &[n, v] : M.functions) v = make_unique(v);
  for (auto &[_, cls] : M.classes)
    for (auto &[n, v] : cls.methods) v = make_unique(v);
}

// ------------------------------------------------

//
void worker_t::separate_properties() {

  auto &M = this->module_info;
  if (not M.get_set_as_properties) return;

  for (auto &[_, cls] : M.classes) {
    decltype(cls.methods) methods2 = {};
    for (auto &[name, v] : cls.methods) {
      if ((v.size() == 1) and (v[0].ptr->getNumParams() == 0)) {
        auto *m = v[0].as_method();
        EXPECTS(m);
        if (not m->getReturnType()->isVoidType()) {
          cls.properties.insert({name, cls_info_t::property{v[0], {}}});
          continue;
        }
      }
      methods2.emplace(name, std::move(v));
    }
    cls.methods = std::move(methods2);
  }
}
// ------------------------------------------------

int get_guard_number(clang::Decl const *d) {
  std::regex const re{R"RAW(c2py_guard_(.*))RAW"};
  for (auto &attr : d->getAttrs()) {
    if (auto an = llvm::dyn_cast_or_null<clang::AnnotateAttr>(attr)) {
      std::smatch m;
      auto anno = std::string{an->getAnnotation()};
      if (std::regex_match(anno, m, re))
        // The first sub_match is the whole string; the next
        // sub_match is the first parenthesized expression.
        if (m.size() == 2) { llvm::errs() << " GAUARD = " << m[1].str(); }
    }
  }
  return 0;
}
// ------------------------------------------------

class check_return_visitor : public clang::RecursiveASTVisitor<check_return_visitor> {
  fnt_ptr_t f;

  public:
  explicit check_return_visitor(fnt_ptr_t f) : f{f} {}

  bool VisitStmt(clang::Stmt *s) {
    // Check only the return Statement in the tree
    if (auto *ret = llvm::dyn_cast_or_null<clang::ReturnStmt>(s); ret) {
      auto *ret_value = ret->getRetValue();
      // peel off the ImplicitCastExpr
      while (auto *decl = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(ret_value)) { ret_value = decl->getSubExpr(); }
      // do we return this->something ? [ in fact A-> something and A == this]. If not : error
      if (auto *ex = llvm::dyn_cast_or_null<clang::MemberExpr>(ret_value); not(ex and llvm::dyn_cast_or_null<clang::CXXThisExpr>(ex->getBase()))) {
        clu::emit_error(f->getReturnTypeSourceRange().getBegin(), f->getASTContext(),
                        "c2py: Can not be converted from C++ to Python. I can not check that this method returns a member of `this`.");
        clu::emit_error(ret->getBeginLoc(), f->getASTContext(), "c2py: ... due to this return statement.");
      }
    }
    return true;
  }
};

// Check convertibility of parameters, return type, and fields
void worker_t::check_convertibility() {

  // ordred list of all wrapped class pointer
  std::vector<cls_ptr_t> wcls;
  for (auto const &[n, clsi] : this->module_info.classes) wcls.push_back(clsi.ptr);
  std::sort(wcls.begin(), wcls.end());

  auto clean_type1 = [](clang::QualType const &ty) -> clang::CXXRecordDecl const * {
    if (auto *cls = ty->getPointeeCXXRecordDecl()) return cls;
    if (auto *cls = ty->getAsCXXRecordDecl()) return cls;
    return nullptr;
  };

  auto is_wrapped = [&wcls, &clean_type1](auto &ty) {
    auto *cls = clean_type1(ty);
    return cls and std::binary_search(wcls.begin(), wcls.end(), cls);
  };

  auto checkf = [this, &is_wrapped](fnt_ptr_t const &f) {
    for (auto i : itertools::range(f->getNumParams())) {
      auto *p = f->getParamDecl(i);
      auto ty = p->getOriginalType();
      if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->IsConvertiblePy2C, this->ci)) and (not is_wrapped(ty)))
        clu::emit_error(p, "c2py: Can not be converted from python to C++");
    }
    auto ty = f->getReturnType();
    if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->IsConvertibleC2Py, this->ci)) and (not is_wrapped(ty)))
      clu::emit_error(f, "c2py: Can not be converted from C++ to python");
    else {
      if (ty->isReferenceType()) { // further checks if we return a reference
                                   // it must be a method, and we only return this->a_member;
                                   // everything else is rejected
        if (auto m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f); !m) {
          clu::emit_error(f, "c2py: Can not be converted from C++ to Python. Only methods can return a reference.");
        } else {
          auto visitor = check_return_visitor{f};
          visitor.TraverseStmt(m->getBody());
        }
      }
    }
  };

  auto checkv = [&checkf](std::vector<fnt_info_t> const &flist) {
    for (auto const &f : flist) checkf(f.ptr);
  };

  auto checkm = [&checkv](std::map<str_t, std::vector<fnt_info_t>> const &mflist) {
    for (auto &[n, v] : mflist) checkv(v);
  };

  checkm(this->module_info.functions);
  for (auto &[_, cls] : this->module_info.classes) {
    checkm(cls.methods);
    for (auto &[n, p] : cls.properties) {
      checkf(p.getter.ptr);
      checkv(p.setters);
    }
    checkv(cls.constructors);
    checkv(cls.getitems);
    // checkv(cls.setitems); //FIXME : do not check the reutrn type here, as it can be ref

    // fields
    for (auto *f : cls.fields) {
      auto ty = f->getType();
      if (is_wrapped(ty)) continue; // the ty will be wrapped in the module we generate now.
      if (not clu::satisfy_concept(ty, this->IsConvertiblePy2C, this->ci)) clu::emit_error(f, "c2py: Can not be converted from python to C++");
      if (not clu::satisfy_concept(ty, this->IsConvertibleC2Py, this->ci)) clu::emit_error(f, "c2py: Can not be converted from C++ to python");
    }
  }
}

// -------------------

void worker_t::run() {
  this->get_additional_methods();
  this->scan_class_and_bases_elements();
  this->prepare_methods();
  // must be after prepare_methods
  this->remove_multiple_decl();
  this->separate_properties();
  this->check_convertibility();
}
