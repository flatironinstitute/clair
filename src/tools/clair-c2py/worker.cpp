#include "./worker.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <filesystem>

#include "llvm/ADT/DenseSet.h"

#include <itertools/itertools.hpp>
#include "utility/macros.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include "clu/misc.hpp"
#include "clu/concept.hpp"

static const struct {
  util::logger error    = util::logger{&std::cout, "-- ", "\033[1;33mError:  \033[0m"};
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
} logs;

//--------------------------------------------------------

worker_t::worker_t(clang::CompilerInstance *ci, configuration const &config) : ci{ci}, config{config} {

  auto p                           = std::filesystem::absolute(ci->getFrontendOpts().Inputs[0].getFile().str());
  module_info.sourcefile           = str_t{p.string()};
  module_info.module_name          = str_t{p.stem()};
  module_info.sourcefile_full_stem = p.parent_path() / p.stem();
  module_info.package_name         = config.package_name;
  module_info.documentation        = config.documentation;

  // Validity of the regex is checked in the configuration constructor
  if (not config.reject_names.empty()) this->reject_names = llvm::Regex(config.reject_names);
}

// -----------------------------

bool worker_t::is_rejected(clang::Decl const *decl, util::logger const *log) {
  auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
  if (!named_decl) return true; // no name -> reject
  auto name = named_decl->getQualifiedNameAsString();
  // is annoted explicitely -> reject
  if (clu::has_annotation(named_decl, "c2py_ignore")) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "C2PY_IGNORE"));
    return true;
  }
  // matches the regex -> reject
  if (reject_names && reject_names->match(name)) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "reject_names"));
    return true;
  }
  return false;
}
// ------------------------------------------------

// check if function parameter and return type are convertible.
bool worker_t::check_convertibility(clang::FunctionDecl const *f, bool test_return_type) const {
  bool emit_error = true; // TODO: make it depends on options and regex
  bool ok         = true;
  for (auto i : itertools::range(f->getNumParams())) {
    auto *p = f->getParamDecl(i);
    auto ty = p->getType();
    if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->IsConvertiblePy2C, this->ci)) and (not this->module_info.is_wrapped(ty))) {
      if (emit_error) clu::emit_error(p, "c2py: Can not convert this argument from python to C++");
      ok = false;
    }
  }
  if (test_return_type) {
    auto ty = f->getReturnType();
    if (ty->isPointerType() and (ty->getPointeeType().getAsString() != "PyObject")) {
      if (emit_error) clu::emit_error(f, "c2py: Can not convert a raw C++ pointer to python");
      ok = false;
    } else if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->IsConvertibleC2Py, this->ci))
               and (not this->module_info.is_wrapped(ty))) {
      if (emit_error) clu::emit_error(f, "c2py: Can not convert this return type from C++ to python");
      ok = false;
    }
  }
  return ok;
}
//--------------------------------------------------------

str_t worker_t::get_python_name(clang::CXXRecordDecl const *cls) const {
  if (auto rename = clu::get_annotation_value(cls, "c2py_rename"))
    return *rename;
  else
    return util::camel_case(cls->getNameAsString());
}

str_t worker_t::get_python_name(clang::FunctionDecl const *f) const {
  str_t py_name = f->getNameAsString();
  if (auto rename = clu::get_annotation_value(f, "c2py_rename")) py_name = *rename;
  return py_name;
}
//--------------------------------------------------------

void worker_t::analyze_one_method(clang::FunctionDecl const *f, cls_info_t &cls_info, cls_ptr_t cls) {

  if (f->isDeleted()) return;
  auto *m = llvm::dyn_cast<clang::CXXMethodDecl>(f);
  if (!m) return;
  if (llvm::isa<clang::CXXDestructorDecl>(m)) return;                   // no destructors
  if (m->isMoveAssignmentOperator()) return;                            // no move assign
  if (auto *ctr = llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(m); //
      ctr and ctr->isCopyOrMoveConstructor())
    return; // no move or copy constructor

  auto name = m->getNameAsString();

  // ---- constructors
  if (llvm::isa<clang::CXXConstructorDecl>(m)) {
    const bool is_base_class = (cls != cls_info.ptr);
    if (not is_base_class and check_convertibility(m)) cls_info.constructors.push_back({m});
    return;
  }
  // ---- operators : keep only [] and ()
  if (name.starts_with("operator")) {
    if (name == "operator[]") {
      ASSERT(m);
      // Do not check the return type, only the parameters for the setitem, it is coded differently
      // than other functions
      if (check_convertibility(m, m->isConst())) (m->isConst() ? cls_info.getitems : cls_info.setitems).push_back({m});
    } else if (name == "operator()") {
      if (check_convertibility(m)) cls_info.methods["__call__"].push_back({m});
    }
    // all other operators are ignored
    return;
  }

  // ---- special cases first
  if (name == "size" and (f->getNumParams() == 0)) {
    cls_info.has_size_method = true;
    return;
  }

  // ---- iterator methods : begin, end and co
  if (name == "begin" or name == "end" or name == "cend" or name == "cbegin") {
    cls_info.has_iterator = true;
    return; // do not wrap these methods
  }

  // generic case
  if (check_convertibility(m)) cls_info.methods[get_python_name(m)].push_back({m});
}

// ---------     MAKE UNIQUE functions--------------
// Make a list of function unique, keeping the order
std::vector<fnt_info_t> make_unique(std::vector<fnt_info_t> const &flist) {
  llvm::DenseSet<const clang::FunctionDecl *> seen; // LLVM recommended replacement of std::set
  std::vector<fnt_info_t> res;
  seen.reserve(flist.size());
  res.reserve(flist.size());

  for (const auto &f : flist) {
    if (seen.insert(f.ptr->getMostRecentDecl()).second) // first time we see this decl
      res.push_back(f);
  }
  return res;
}

// -----------------------
// Takes a list of methods, and return a list without const/non const method duplication
// Choose the non-const version if there is both.
std::vector<fnt_info_t> rm_const_overloads(std::vector<fnt_info_t> const &mlist) {

  // Extract parameter types and constness
  auto extract_signature = [](fnt_info_t const &fi) {
    llvm::SmallVector<clang::QualType> params;
    params.reserve(fi.ptr->getNumParams());
    for (auto const &p : fi.ptr->parameters()) params.push_back(p->getType());
    auto *method = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(fi.ptr);
    return std::pair{std::move(params), method && method->isConst() ? 0 : 1};
  };

  // Create signatures for all methods
  std::vector<std::pair<llvm::SmallVector<clang::QualType>, int>> signatures;
  std::transform(mlist.begin(), mlist.end(), std::back_inserter(signatures), extract_signature);

  // Remove duplicates based on parameter types
  std::vector<int> idx(signatures.size());
  std::iota(idx.begin(), idx.end(), 0);
  std::sort(idx.begin(), idx.end(), [&signatures](int i, int j) { return signatures[i] < signatures[j]; });
  idx.erase(std::unique(idx.begin(), idx.end(), [&signatures](int i, int j) { return signatures[i].first == signatures[j].first; }), idx.end());
  std::sort(idx.begin(), idx.end());

  // replace with ranges when widely supported
  std::vector<fnt_info_t> result;
  result.reserve(idx.size());
  for (int i : idx) result.push_back(mlist[i]);

  return result;
}

// ------------------------------------------------

// Given cls, stores its methods and friend functions
void worker_t::scan_class_elements(cls_info_t &cls_info, cls_ptr_t cls) {

  for (clang::Decl *decl : cls->decls()) { // all declarations in the class
    if (decl->getAccess() != clang::AS_public) continue;
    if (this->is_rejected(decl, &logs.rejected)) continue;

    // --------  method
    if (auto *m = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
      analyze_one_method(m, cls_info, cls);
    }
    // -------- templated method
    else if (auto *m_tpl = llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
      for (auto *spec : m_tpl->specializations())
        if (auto *info = spec->getTemplateSpecializationInfo(); info and info->isExplicitInstantiationOrSpecialization())
          analyze_one_method(spec, cls_info, cls);
    }
    // -------- fields
    else if (auto *f = llvm::dyn_cast<clang::FieldDecl>(decl)) {
      auto ty = f->getType();
      if (not this->module_info.is_wrapped(ty)) {
        if (not clu::satisfy_concept(ty, this->IsConvertiblePy2C, this->ci)) clu::emit_error(f, "c2py: Can not be converted from python to C++");
        if (not clu::satisfy_concept(ty, this->IsConvertibleC2Py, this->ci)) clu::emit_error(f, "c2py: Can not be converted from C++ to python");
      }
      cls_info.fields.push_back(f);
    }
  }
}

// ------------------------------------------------------

void worker_t::scan_class_and_bases_elements(cls_info_t &cls_info) {

  // h5
  cls_info.has_hdf5 = HasHdf5 and clu::satisfy_concept(cls_info.ptr, HasHdf5, this->ci);

  // Serialization
  if (clu::satisfy_concept(cls_info.ptr, this->HasSerializeLikeBoost, this->ci))
    cls_info.serialization = Serialization::Tuple;
  else if (cls_info.has_hdf5)
    cls_info.serialization = Serialization::H5;

  // Get the methods and fields of the class
  scan_class_elements(cls_info, cls_info.ptr);

  // We loop on base classes which are not wrapped
  // an authorize 1 base class to be wrapped (Python C API limitation)
  for (auto b : cls_info.ptr->bases()) {
    if (b.getAccessSpecifier() != clang::AccessSpecifier::AS_public) continue; // only public bases
    auto *c = b.getType()->getAsCXXRecordDecl();
    if (not this->module_info.is_wrapped(b.getType())) {
      // We merge the element of the base into the class in progress.
      scan_class_elements(cls_info, c);
    } else {
      if (cls_info.base != nullptr) clu::emit_error(cls_info.ptr, "This class has more than one bases to wrap");
      cls_info.base = c;
    }
  }
  // finally we remove the const/non const duplicate in methods
  for (auto &[n, v] : cls_info.methods) {
    v = make_unique(v);
    v = rm_const_overloads(v);
  }
}

// ------------------------------------------------
void worker_t::separate_properties(cls_info_t &cls_info) {
  // if the method has no argument and is not void (?)
  // we remove it as method, and insert it in the property list
  std::erase_if(cls_info.methods, [&cls_info](auto &&p) -> bool {
    auto &[name, v] = p;
    if ((v.size() == 1) and (v[0].ptr->getNumParams() == 0)) {
      if (auto *m = v[0].as_method(); m and not m->getReturnType()->isVoidType()) {
        cls_info.properties.insert({name, cls_info_t::property{v[0], {}}});
        return true; // remove
      }
    }
    return false; // default: do not remove
  });
}

// -------------------

void worker_t::run() {

  for (auto &[_, v] : this->module_info.functions) v = make_unique(v);

  for (auto &[_, cls_info] : this->module_info.classes) {
    this->scan_class_and_bases_elements(cls_info);
    if (config.wrap_no_arg_methods_as_properties) this->separate_properties(cls_info);

    // Checks
    if (cls_info.constructors.empty() and not cls_info.synthetize_dict_attribute()
        and not clu::satisfy_concept(cls_info.ptr, this->HasNonDeletedDefaultConstructor, this->ci))
      clu::emit_error(cls_info.ptr, "This class has no wrapped constructor and is not default constructible.");
  }
}
