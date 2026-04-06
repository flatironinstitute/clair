#include "fnt_tools.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include <itertools/itertools.hpp>
#include <clang/AST/DeclTemplate.h>
#include "clu/fullqualifiedname.hpp"
#include "clu/misc.hpp"

using util::join;

// ========== A few helper functions to extract params of a function ===========

// Returns the parameter name, generating a synthetic _p_N if unnamed.
static str_t param_name(clang::ParmVarDecl const *p, int i) {
  auto n = p->getNameAsString();
  return n.empty() ? fmt::format("_p_{}", i) : n;
}

// ------------------------------

// e.g. f(A a, B b = 2) --->   a,b
str_t fnt_params(fnt_ptr_t f) {
  return join(itertools::range(f->getNumParams()), [f](int i) { return param_name(f->getParamDecl(i), i); }, ',');
}

// ------------------------------

// Fully qualified type of the i-th parameter.
static str_t fnt_param_type(fnt_ptr_t f, int i) { return clu::get_fully_qualified_name(f->getParamDecl(i)->getType(), f->getASTContext()); }

// ------------------------------

// e.g. f(A a, B b = 2) --->   A, B
str_t fnt_paramtypes(fnt_ptr_t f) {
  return join(itertools::range(f->getNumParams()), [f](int i) { return fnt_param_type(f, i); }, ',');
}

// ------------------------------

// e.g. f(A a, B b = 2) --->   A a, B b
str_t fnt_param_with_types(fnt_ptr_t f) {
  return join(itertools::range(f->getNumParams()), [f](int i) { return fnt_param_type(f, i) + ' ' + param_name(f->getParamDecl(i), i); }, ',');
}

// ------------------------------

// same with tpl parameters
str_t fnt_tparams(fnt_ptr_t f) {
  clang::ASTContext *ctx = &f->getASTContext();
  return join(f->getTemplateSpecializationArgs()->asArray(), [&ctx](auto &&ta) { return clu::get_name_of_TemplateArgument(ta, ctx); }, ',');
}

// ------------------------------

// A , separated list of parameters names, with default
// e.g. f(A a, B b = 2) --->   "a", "b"_a = 2
str_t fnt_params_with_default(clang::FunctionDecl const *f) {

  // best_redecl() in worker.cpp already picks the declaration with defaults/names,
  // but for template specializations we need the original template declaration
  // for parameter names and default argument expressions.
  // However, for resolving types (e.g. in braced-init defaults), we need the
  // instantiated function's parameter types, not the unresolved template parameter types.
  clang::FunctionDecl const *f_for_types = f;
  if (auto *info = f->getTemplateSpecializationInfo(); info and info->isExplicitInstantiationOrSpecialization())
    f = info->getTemplate()->getTemplatedDecl();
  // For members of class template instantiations, Clang does not propagate
  // default arguments to the instantiated parameters. Retrieve the pattern
  // (the member in the primary class template) which carries the defaults.
  else if (auto *pattern = f->getInstantiatedFromMemberFunction())
    f = pattern;

  // extract the default argument of a parameter declaration
  // p_type is the resolved parameter type from the instantiated function
  auto extract_default_argument = [](clang::ParmVarDecl const *p, clang::QualType p_type) -> str_t {
    clang::Expr const *defarg = p->getDefaultArg();
    EXPECTS(defarg);
    defarg = defarg->IgnoreParenImpCasts();

    //if (not defarg) return {};
    clang::ASTContext *ctx = &p->getASTContext();

    // first if implicit conversion, remove this layer
    if (auto *decl = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(defarg)) { defarg = decl->getSubExpr(); }

    // If a reference to a variable
    if (auto *decl = llvm::dyn_cast_or_null<clang::DeclRefExpr>(defarg)) { return decl->getFoundDecl()->getQualifiedNameAsString(); }

    // default solution : just extract the source code and clean it a bit
    auto s = clu::get_source_range_as_string(p->getDefaultArgRange(), ctx);
    //  strip leading '='  and spaces
    size_t i = 0;
    while (i < s.size() && (isspace((unsigned char)s[i]) || (s[i] == '='))) ++i;
    s = s.substr(i);
    // If the default contains a braced init (bare '{...}' or 'Type{...}'),
    // replace everything before '{' with the fully qualified type name.
    // This handles both bare braced-init-lists and Type{args} where the type may contain
    // unqualified namespace-scoped aliases (e.g. myint_t instead of ns::myint_t).
    // Use the resolved type (p_type) rather than p->getType() which may be an
    // unresolved template parameter (e.g. type-parameter-0-2) for specializations.
    if (auto pos = s.find('{'); pos != str_t::npos) {
      // Strip reference and cv-qualifiers to get a constructible type for braced-init.
      // e.g. "const array_const_view<dcomplex, 3> &" -> "array_const_view<dcomplex, 3>"
      s = clu::get_fully_qualified_name(p_type.getNonReferenceType().getUnqualifiedType(), *ctx) + s.substr(pos);
    }
    return s;
  };
  // --------

  // prepare the string of "A"_a = A_default, chain.
  str_t pyargs = join( // NB always a , at the front ...
     itertools::range(f->getNumParams()),
     [f, f_for_types, &extract_default_argument](int i) {
       clang::ParmVarDecl const *p = f->getParamDecl(i);
       // FIXME : rewrite with 2 fromat...
       // FIXME : start with , if anything : do NOT join ...
       auto res = fmt::format(R"RAW( "{0}")RAW", p->getNameAsString());
       if (p->hasDefaultArg()) { res += "_a = " + extract_default_argument(p, f_for_types->getParamDecl(i)->getType()); }
       return res;
     },
     ',');

  return pyargs;
}
