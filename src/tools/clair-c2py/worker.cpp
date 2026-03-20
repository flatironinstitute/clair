#include "./worker.hpp"

#include <iostream>
#include <filesystem>

#include "llvm/ADT/DenseSet.h"
#include <clang/AST/RecursiveASTVisitor.h>

#include <itertools/itertools.hpp>
#include "utility/macros.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include "clu/misc.hpp"
#include "clu/concept.hpp"
#include "data.hpp"

static const struct {
  util::logger rejected = util::logger{&std::cout, "-- ", "\033[1;33mRejecting: \033[0m"};
} logs;

// ------------------------------------------------

// Map operator name + arity to OpKind. Returns std::nullopt for unsupported operators.
// Arity is the number of operands (1 = unary, 2 = binary).
static std::optional<OpKind> operator_name_to_kind(std::string_view name, int arity) {
  if (name == "operator+" and arity == 2) return OpKind::Add;
  if (name == "operator-" and arity == 2) return OpKind::Sub;
  if (name == "operator+" and arity == 1) return OpKind::Pos;
  if (name == "operator-" and arity == 1) return OpKind::Neg;
  if (name == "operator*") return OpKind::Mul;
  if (name == "operator/") return OpKind::Div;
  if (name == "operator==") return OpKind::Eq;
  if (name == "operator!=") return OpKind::Ne;
  if (name == "operator<") return OpKind::Lt;
  if (name == "operator>") return OpKind::Gt;
  if (name == "operator<=") return OpKind::Le;
  if (name == "operator>=") return OpKind::Ge;
  return std::nullopt;
}

// ------------------------------------------------
// Validates that every return statement in a reference-returning method
// returns a direct (or inherited) member of `this`.
//
// The visitor traverses the full body and emits an error for any
// return statement whose expression is not of that form.  This covers all
// control-flow paths (if/else branches, early returns, etc.).
//
// Expected AST shape for a valid return:
//   ReturnStmt
//     ImplicitCastExpr*          (zero or more, e.g. lvalue-to-rvalue)
//       MemberExpr
//         ImplicitCastExpr*      (zero or more, e.g. derived-to-base cast)
//           CXXThisExpr
//
class check_return_visitor : public clang::RecursiveASTVisitor<check_return_visitor> {
  fnt_ptr_t f;

  public:
  explicit check_return_visitor(fnt_ptr_t f) : f{f} {}

  // Do not descend into nested scopes (lambdas, local classes) whose
  // return statements belong to the inner function, not to the method.
  bool TraverseLambdaExpr(clang::LambdaExpr *) { return true; }
  bool TraverseCXXRecordDecl(clang::CXXRecordDecl *) { return true; }

  bool VisitReturnStmt(clang::ReturnStmt *ret) {
    // Peel implicit casts on the returned expression (e.g. lvalue-to-rvalue).
    clang::Expr const *ret_value = ret->getRetValue();
    while (auto *ice = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(ret_value)) ret_value = ice->getSubExpr();

    // The expression must be a member access …
    auto *ex = llvm::dyn_cast_or_null<clang::MemberExpr>(ret_value);

    // Take the base of the outermost MemberExpr (e.g. base of `.something` is `b`).
    // If ex was null (no MemberExpr found at all), start with nullptr.
    clang::Expr const *base = ex ? ex->getBase() : nullptr;
    // Peel any ImplicitCastExpr wrappers (e.g. derived-to-base casts for inherited members).
    // After this, base is either CXXThisExpr, another MemberExpr (chained access), or something else.
    while (auto *ice = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(base)) base = ice->getSubExpr();

    // Accept only `this->direct_member`.  Anything else is rejected: nullptr (no MemberExpr),
    // another MemberExpr (chained access like `b.something` where b is a member), a DeclRefExpr
    // (local variable or global), etc.
    // Caveat: `this->member.submember` is also rejected even though it is safe, because the
    // chain is not walked.
    // Fix if needed: replace the ImplicitCastExpr loop above with a loop that also
    // descends through MemberExpr nodes until the root base is reached.
    if (not llvm::dyn_cast_or_null<clang::CXXThisExpr>(base)) {
      clu::emit_error(f->getReturnTypeSourceRange().getBegin(), f->getASTContext(),
                      "c2py: Can not be converted from C++ to Python. I can not check that this method returns a member of `this`.");
      clu::emit_error(ret->getBeginLoc(), f->getASTContext(), "c2py: ... due to this return statement.");
    }
    return true;
  }
};

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
  bool ok = true;
  for (auto i : itertools::range(f->getNumParams())) {
    auto *p = f->getParamDecl(i);
    auto ty = p->getType();
    if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->concepts.IsConvertiblePy2C, this->ci)) and (not this->module_info.is_wrapped(ty))) {
      clu::emit_error(p, "c2py: Can not convert this argument from python to C++");
      ok = false;
    }
  }
  if (test_return_type) {
    auto ty = f->getReturnType();
    if (ty->isPointerType() and (ty->getPointeeType().getAsString() != "PyObject")) {
      clu::emit_error(f, "c2py: Can not convert a raw C++ pointer to python");
      ok = false;
    } else if ((not ty->isVoidType()) and (not clu::satisfy_concept(ty, this->concepts.IsConvertibleC2Py, this->ci))
               and (not this->module_info.is_wrapped(ty))) {
      clu::emit_error(f, "c2py: Can not be converted from C++ to python");
      ok = false;
    } else {
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
  }
  return ok;
}
//--------------------------------------------------------

str_t worker_t::get_python_name(clang::CXXRecordDecl const *cls) const {
  if (auto rename = clu::get_annotation_value(cls, "c2py_rename"))
    return *rename;
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
      // Do not check the return type, only the parameters for the setitem, it is coded differently
      // than other functions
      if (check_convertibility(m, m->isConst())) (m->isConst() ? cls_info.getitems : cls_info.setitems).push_back({m});
    } else if (name == "operator()") {
      if (check_convertibility(m)) cls_info.methods["__call__"].push_back({m});
    } else {
      analyze_operator(f);
    }
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

  // ---- explicit property annotations
  if (auto prop_name = clu::get_annotation_value(m, "c2py_property_get")) {
    if (m->getNumParams() != 0)
      clu::emit_error(m, "c2py: C2PY_PROPERTY_GET method must take no parameters");
    else if (m->getReturnType()->isVoidType())
      clu::emit_error(m, "c2py: C2PY_PROPERTY_GET method must not return void");
    else if (check_convertibility(m))
      cls_info.properties[*prop_name].getter = {.ptr = m};
    return;
  }
  if (auto prop_name = clu::get_annotation_value(m, "c2py_property_set")) {
    if (check_convertibility(m)) cls_info.properties[*prop_name].setters.push_back({.ptr = m});
    return;
  }

  // wrap_no_arg_methods_as_properties: treat no-arg non-void methods as read-only properties.
  if (config.wrap_no_arg_methods_as_properties and m->getNumParams() == 0 and not m->getReturnType()->isVoidType()) {
    if (check_convertibility(m)) cls_info.properties[get_python_name(m)].getter = {.ptr = m};
    return;
  }

  // generic case
  if (check_convertibility(m)) cls_info.methods[get_python_name(m)].push_back({m});
}

// ---------     MAKE UNIQUE functions--------------
// Among all redeclarations of f, pick the best one:
// 1. Prefer a redecl with default arguments (at most one exists per C++ rules)
// 2. Otherwise prefer a redecl where all parameters are named
// 3. Fall back to the most recent redecl
const clang::FunctionDecl *best_redecl(const clang::FunctionDecl *f) {
  const clang::FunctionDecl *with_names = nullptr;
  for (auto *redecl : f->redecls()) {
    auto *r = llvm::dyn_cast<clang::FunctionDecl>(redecl);
    if (not r) continue;
    if (llvm::any_of(r->parameters(), [](auto *p) { return p->hasDefaultArg(); })) return r;
    if (not with_names and llvm::all_of(r->parameters(), [](auto *p) { return !p->getName().empty(); })) with_names = r;
  }
  return with_names ? with_names : f->getMostRecentDecl();
}

// Make a list of function unique, keeping the order.
// For each group of redeclarations, pick the best one (with defaults or named params).
std::vector<fnt_info_t> make_unique(std::vector<fnt_info_t> const &flist) {
  llvm::DenseSet<const clang::FunctionDecl *> seen; // LLVM recommended replacement of std::set
  std::vector<fnt_info_t> res;
  seen.reserve(flist.size());
  res.reserve(flist.size());

  for (const auto &f : flist) {
    if (seen.insert(f.ptr->getMostRecentDecl()).second) {
      auto *best = best_redecl(f.ptr);
      res.push_back({.ptr = best, .rewrite = f.rewrite, .parent_class = f.parent_class});
    }
  }

  return res;
}

// -----------------------
// Takes a list of methods, and return a list without const/non const method duplication.
// When both a const and non-const method share the same parameter types, keep only the non-const version.
// Preserves original order.
std::vector<fnt_info_t> rm_const_overloads(std::vector<fnt_info_t> const &mlist) {

  auto get_param_types = [](fnt_info_t const &fi) {
    llvm::SmallVector<clang::QualType> params;
    params.reserve(fi.ptr->getNumParams());
    for (auto const *p : fi.ptr->parameters()) params.push_back(p->getType());
    return params;
  };

  // For each param signature, record the index of the preferred overload (non-const wins).
  std::map<llvm::SmallVector<clang::QualType>, size_t> best;
  for (size_t i = 0; i < mlist.size(); ++i) {
    auto params   = get_param_types(mlist[i]);
    auto *method  = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(mlist[i].ptr);
    bool is_const = method && method->isConst();
    auto [it, inserted] = best.try_emplace(std::move(params), i);
    if (!inserted && !is_const) it->second = i; // non-const wins
  }

  // Collect winners in original order.
  llvm::DenseSet<size_t> winner_indices;
  for (auto const &[_, idx] : best) winner_indices.insert(idx);

  std::vector<fnt_info_t> result;
  result.reserve(winner_indices.size());
  for (size_t i = 0; i < mlist.size(); ++i)
    if (winner_indices.contains(i)) result.push_back(mlist[i]);
  return result;
}

// ------------------------------------------------

void worker_t::analyze_operator(clang::FunctionDecl const *f) {

  auto name    = f->getNameAsString();
  auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(f);

  // Compute the operand arity (including implicit this for methods)
  int arity = int(f->getNumParams()) + (method ? 1 : 0);
  if (arity < 1 or arity > 2) return;

  auto op = operator_name_to_kind(name, arity);
  if (not op) return;

  if (not check_convertibility(f)) return;

  // Build the full argument type list
  std::vector<clang::QualType> args;
  if (method) args.push_back(method->getThisType()->getPointeeType().getUnqualifiedType());
  for (unsigned i = 0; i < f->getNumParams(); ++i)
    args.push_back(f->getParamDecl(i)->getType().getNonReferenceType().getUnqualifiedType());

  // Associate with the class of the first argument; fall back to second if first is not wrapped
  auto *cli = module_info.get_wrapped_cls_info(args[0]);
  if (not cli and args.size() > 1) cli = module_info.get_wrapped_cls_info(args[1]);
  if (cli) cli->operators[*op].push_back(std::move(args));
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
        if (not clu::satisfy_concept(ty, this->concepts.IsConvertiblePy2C, this->ci)) clu::emit_error(f, "c2py: Can not be converted from python to C++");
        if (not clu::satisfy_concept(ty, this->concepts.IsConvertibleC2Py, this->ci)) clu::emit_error(f, "c2py: Can not be converted from C++ to python");
      }
      cls_info.fields.push_back(f);
    }
  }
}

// ------------------------------------------------------

void worker_t::scan_class_and_bases_elements(cls_info_t &cls_info) {

  // h5
  cls_info.has_hdf5 = concepts.HasHdf5 and clu::satisfy_concept(cls_info.ptr, concepts.HasHdf5, this->ci);

  // Serialization
  if (clu::satisfy_concept(cls_info.ptr, this->concepts.HasSerializeLikeBoost, this->ci))
    cls_info.serialization = Serialization::Tuple;
  else if (cls_info.has_hdf5)
    cls_info.serialization = Serialization::H5;

  // Get the methods and fields of the class
  scan_class_elements(cls_info, cls_info.ptr);

  // We loop on base classes which are not wrapped
  // and authorize 1 base class to be wrapped (Python C API limitation)
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

// -------------------

void worker_t::run() {

  for (auto &[_, v] : this->module_info.functions) v = make_unique(v);

  for (auto &[_, cls_info] : this->module_info.classes) {
    this->scan_class_and_bases_elements(cls_info);

    // Checks
    if (cls_info.constructors.empty() and not cls_info.synthetize_dict_attribute()
        and not clu::satisfy_concept(cls_info.ptr, this->concepts.HasNonDeletedDefaultConstructor, this->ci))
      clu::emit_error(cls_info.ptr, "This class has no wrapped constructor and is not default constructible.");
  }
}
