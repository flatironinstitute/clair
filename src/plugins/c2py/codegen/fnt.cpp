#include <algorithm>
#include <numeric>
#include "./fnt.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include <itertools/itertools.hpp>
#include "clu/fullqualifiedname.hpp"
#include "clu/misc.hpp"
#include "utility/logger.hpp"
#include <clang/AST/DeclTemplate.h>
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger fun   = util::logger{&std::cout, "-- ", "\033[1;32mFunction: \033[0m"};
  util::logger meth  = util::logger{&std::cout, "-- ", "\033[1;32m  Method: \033[0m"};
  util::logger fun_c = util::logger{&std::cout, "-- ", "\033[1;32m        . \033[0m"};
  //util::logger method = util::logger{&std::cout, "-- ", "\033[1;32m      -- method: \033[0m"};
  //util::logger constructors = util::logger{&std::cout, "-- ", "\033[1;34m  Constructor: \033[0m"};
} logs;

// ---------------------------------------------------------------

str_t fnt_params(fnt_ptr_t f) {
  return join(
     itertools::range(f->getNumParams()), [f](int i) { return f->getParamDecl(i)->getNameAsString(); }, ',');
}

// ---------------------------------------------------------------

str_t fnt_params_with_default(clang::FunctionDecl const *f) {

  // Find the only declaration which will have default argument if any
  for (auto *redecl : f->redecls()) {
    if (auto *f2 = llvm::dyn_cast_or_null<clang::FunctionDecl>(redecl); //
        f2 and llvm::any_of(f2->parameters(), [](auto *p) { return p->hasDefaultArg(); })) {
      f = redecl;
      break; // only one decl with default argument is permitted
    }
  }

  // extract the default argument of a parameter declaration
  auto extract_default_argument = [](clang::ParmVarDecl const *p) -> str_t {
    clang::Expr const *defarg = p->getDefaultArg();
    EXPECTS(defarg);
    if (not defarg) return {};
    clang::ASTContext *ctx = &p->getASTContext();

    // first if implicit conversion, remove this layer
    if (auto *decl = llvm::dyn_cast_or_null<clang::ImplicitCastExpr>(defarg)) { defarg = decl->getSubExpr(); }

    // If a reference to a variable
    if (auto *decl = llvm::dyn_cast_or_null<clang::DeclRefExpr>(defarg)) { return decl->getFoundDecl()->getQualifiedNameAsString(); }

    // default solution : just extract the source code.
    return clu::get_source_range_as_string(p->getDefaultArgRange(), ctx);
  };
  // --------

  // prepare the string of "A"_a = A_default, chain.
  str_t pyargs = join( // NB always a , at the front ...
     itertools::range(f->getNumParams()),
     [f, &extract_default_argument](int i) {
       clang::ParmVarDecl const *p = f->getParamDecl(i);
       // FIXME : rewrite with 2 fromat...
       // FIXME : start with , if anything : do NOT join ...
       auto res = fmt::format(R"RAW( "{0}")RAW", p->getNameAsString());
       if (p->hasDefaultArg()) { res += "_a = " + extract_default_argument(p); }
       return res;
     },
     ',');

  return pyargs;
}
// ---------------------------------------------------------------

str_t fnt_param_type(fnt_ptr_t f, int i) { return clu::get_fully_qualified_name(f->getParamDecl(i)->getOriginalType(), f->getASTContext()); }

// ---------------------------------------------------------------

str_t fnt_paramtypes(fnt_ptr_t f) {
  return join(
     itertools::range(f->getNumParams()), [f](int i) { return fnt_param_type(f, i); }, ',');
}

// ---------------------------------------------------------------

str_t fnt_param_with_types(fnt_ptr_t f) {
  return join(
     itertools::range(f->getNumParams()), [f](int i) { return fnt_param_type(f, i) + ' ' + f->getParamDecl(i)->getNameAsString(); }, ',');
}

// ---------------------------------------------------------------

str_t fnt_tparams(fnt_ptr_t f) {

  clang::ASTContext *ctx = &f->getASTContext();
  return join(
     f->getTemplateSpecializationArgs()->asArray(), [&ctx](auto &&ta) { return clu::get_name_of_TemplateArgument(ta, ctx); }, ',');
}

// ===================================================================

void codegen::write_dispatch(std::ostream &code, std::ostream &table, std::ostream &doc, std::string const &pyname,
                             std::vector<fnt_info_t> const &flist, clang::CXXRecordDecl const *parent_class, bool enforce_method) {

  static long fun_counter = 0;
  // no // if (flist.empty()) return; // can happen, some function are moved to properties

  // ---- write the dispatcher ----

  if (parent_class)
    logs.meth(fmt::format("{}", pyname));
  else
    logs.fun(fmt::format("{}", pyname));

  code << '\n'
       << fmt::format(R"RAW( // {}  
                             static auto const fun_{} = c2py::dispatcher_f_kw_t{{ )RAW",
                      pyname, fun_counter);

  auto l = [&enforce_method, parent_class](auto &f_info) {
    auto *f    = f_info.ptr;
    auto *m    = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f);
    auto args  = fnt_params_with_default(f);
    auto fname = (m and parent_class ? clu::get_fully_qualified_name(parent_class) + "::" + f->getNameAsString() : f->getQualifiedNameAsString());
    auto cfun_or_cmethod = std::string{enforce_method and (not m) ? "cmethod" : "cfun"};
    if (m and parent_class and m->getParent() != parent_class) // it is a inherited method
      cfun_or_cmethod += "_B<" + clu::get_fully_qualified_name(parent_class) + '>';
    // if m is inherited, we add the <Cls> explicitly to pass Cls to the dispatcher properly

    logs.fun_c(fmt::format("{0}({1})", fname, fnt_param_with_types(f)));

    if (f_info.rewrite)
      return fmt::format(R"RAW( c2py::cfun([]({}) {{ return {}({}); }} {} {}))RAW", fnt_param_with_types(f), fname, fnt_params(f), comma_if(args),
                         args);
    else {
      //if (f->isTemplateInstantiation())
      if (f->getTemplateSpecializationArgs())
        return fmt::format(R"RAW( c2py::{}( &{}<{}> {} {}))RAW", cfun_or_cmethod, fname, fnt_tparams(f), comma_if(args), args);
      else
        return fmt::format(R"RAW( c2py::{}( c2py::cast{}<{}>(&{}) {} {} ))RAW", cfun_or_cmethod,
                           (m ? (m->isStatic() ? "" : (m->isConst() ? "mc" : "m")) : ""), fnt_paramtypes(f), fname, comma_if(args), args);
      // static method -> cast, const method -> castmc, non const, non static method -> castm
    }
  };

  code << join(flist, l, ',') << " };\n";

  // ---- write the doc  ----
  // We generate a simple code in case the flist is of size 1
  // to make it more readable
  if (flist.size() == 1) {
    doc << fmt::format(R"RAW( static const auto doc_d_{0} = fun_{0}.doc({{ R"DOC({1})DOC" }}); )RAW", fun_counter, pydoc(flist[0]));
  } else {

    for (auto const &[i, fi] : itertools::enumerate(flist)) {
      doc << fmt::format(R"RAW( static constexpr auto doc_f_{}_{} = R"DOC({})DOC"; )RAW", fun_counter, i, pydoc(fi));
    }
    doc << fmt::format(R"RAW( static const auto doc_d_{0} = fun_{0}.doc({{ {1} }}); )RAW", fun_counter,
                       join(
                          itertools::range(long(flist.size())), [c = fun_counter](int i) { return fmt::format("doc_f_{}_{}", c, i); }, ','));
  }

  // ---- put if in the table ----
  // the call function are special
  if (pyname == "__call__")
    code << '\n'
         << fmt::format(R"RAW(  template <> inline constexpr ternaryfunc c2py::tp_call<{0}> = c2py::pyfkw<fun_{1}>;  )RAW",
                        parent_class->getQualifiedNameAsString(), fun_counter)
         << '\n';
  else { // generic case
    // is one of the methods static ?
    auto is_static = std::any_of(flist.begin(), flist.end(), [](auto &fi) {
      auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(fi.ptr);
      return (m and m->isStatic());
    });

    // add in the table
    table << fmt::format(R"RAW( {{"{}", (PyCFunction)c2py::pyfkw<fun_{}>, METH_VARARGS | METH_KEYWORDS {}, doc_d_{}.c_str()}}, )RAW", //
                         pyname, fun_counter, (is_static ? "| METH_STATIC" : ""), fun_counter);
  }

  fun_counter++;
}
// ===================================================================

void codegen::write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::vector<fnt_info_t> const &flist) {

  static long counter = 0;
  code << fmt::format(R"RAW( static auto init_{} = c2py::dispatcher_c_kw_t {{ )RAW", counter) << '\n';

  logs.meth("__init__");

  auto l = [&cls_cpp_name](auto &f_info) {
    auto *f    = f_info.ptr;
    auto args  = fnt_params_with_default(f);
    auto targs = fnt_paramtypes(f);
    logs.fun_c(fmt::format("{0}({1})", cls_cpp_name, fnt_param_with_types(f)));

    if (llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(f))
      return fmt::format(R"RAW( c2py::c_constructor<{}{}{}>({}))RAW", cls_cpp_name, comma_if(targs), targs, args);
    else {
      if (f->isTemplateInstantiation())
        return fmt::format(R"RAW( c2py::c_maker<{}>( {}<{}> {} {}))RAW", cls_cpp_name, f->getQualifiedNameAsString(), fnt_tparams(f), comma_if(args),
                           args);
      else
        return fmt::format(R"RAW( c2py::c_maker<{}>( c2py::cast<{}>({}) {} {} ))RAW", cls_cpp_name, fnt_paramtypes(f), f->getQualifiedNameAsString(),
                           comma_if(args), args);
    }
  };

  if (flist.empty()) {
    // no constructor : use default
    code << fmt::format(R"RAW(c2py::c_constructor<{}>())RAW", cls_cpp_name);
    logs.fun_c(fmt::format("{0}() [default]", cls_cpp_name));
  } else
    code << join(flist, l, ',');

  code << "};\n";

  code << fmt::format(R"RAW( template <> constexpr initproc c2py::tp_init<{}> = c2py::pyfkw_constructor<init_{}>;)RAW", //
                      cls_cpp_name, counter);

  counter++;
}
