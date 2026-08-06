#include <algorithm>
#include <numeric>
#include "fnt.hpp"
#include "./utils.hpp"
#include "../wdata.hpp"
#include <fmt/format.h>
using namespace fmt::literals;
#include <itertools/itertools.hpp>
#include "clu/fullqualifiedname.hpp"
#include "clu/misc.hpp"
#include "utility/logger.hpp"
#include "utility/string_tools.hpp"
#include <clang/AST/DeclTemplate.h>
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger fun   = util::logger{"-- ", "\033[1;32mFunction: \033[0m", 1};
  util::logger meth  = util::logger{"-- ", "\033[1;32m  Method: \033[0m", 1};
  util::logger fun_c = util::logger{"-- ", "\033[1;32m        . \033[0m", 2};
  //util::logger method = util::logger{"-- ", "\033[1;32m      -- method: \033[0m", 1};
  //util::logger constructors = util::logger{"-- ", "\033[1;34m  Constructor: \033[0m", 1};
} logs;

// ===================================================================

// Collect C2PY_DEPRECATED_PARAMETER_NAME annotations from all overloads
// and emit .with_deprecated_params({...}) if any are found.
static void emit_deprecated_params(std::ostream &code, std::vector<fnt_info_t> const &flist) {
  std::map<str_t, str_t> deprecated_params;
  for (auto const &f_info : flist) {
    if (auto annot = clu::get_annotation_value(f_info.ptr, "c2py_deprecated_params")) {
      for (auto &pair_str : util::split(*annot, ',')) {
        auto parts = util::split(pair_str, ':');
        if (parts.size() == 2) deprecated_params[util::trim(parts[0])] = util::trim(parts[1]);
      }
    }
  }
  if (not deprecated_params.empty()) {
    code << ".with_deprecated_params({"
         << join(deprecated_params, [](auto const &p) { return fmt::format(R"RAW({{"{}", "{}"}})RAW", p.first, p.second); }, ", ") << "})";
  }
}

// ===================================================================

void codegen::write_dispatch(std::ostream &code, std::ostream &table, std::ostream &doc, std::string const &pyname,
                             std::vector<fnt_info_t> const &flist, clang::CXXRecordDecl const *parent_class, bool enforce_method,
                             std::string const &cls_alias) {

  // no // if (flist.empty()) return; // can happen, some function are moved to properties

  // ---- write the dispatcher ----

  if (parent_class)
    logs.meth(fmt::format("{}", pyname));
  else
    logs.fun(fmt::format("{}", pyname));

  // Stable id derived from a key unique per logical entity:
  //   method      : <parent_class_FQN>::<pyname>
  //   free function: <pyname>
  // Inserting/removing any other function leaves this hash unchanged.
  auto fun_id = id_hash(parent_class ? clu::get_fully_qualified_name(parent_class) + "::" + pyname : pyname);

  code << '\n'
       << fmt::format(R"RAW( // {}
                             static auto const _c2py_fun_{} = c2py::dispatcher_f_kw_t{{ )RAW",
                      pyname, fun_id);

  // Use cls_alias if provided, otherwise fall back to computing the FQN of parent_class.
  auto parent_cls_name = (not cls_alias.empty()) ? cls_alias : (parent_class ? clu::get_fully_qualified_name(parent_class) : str_t{});

  auto l = [&enforce_method, parent_class, &parent_cls_name](fnt_info_t const &f_info) {
    auto *f   = f_info.ptr;
    auto *m   = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(f);
    auto args = fnt_params_with_default(f);
    // Inline friend functions (defined inside a class body) are only found via ADL,
    // so they must be called unqualified. They are not CXXMethodDecls.
    bool is_inline_friend = (not m) and f->getFriendObjectKind() != clang::Decl::FOK_None;
    auto fname            = (m and parent_class ? parent_cls_name + "::" + f->getNameAsString() :
                                                  (is_inline_friend ? f->getNameAsString() : f->getQualifiedNameAsString()));
    auto fname_log = (m and parent_class ? clu::get_fully_qualified_name(parent_class) + "::" + f->getNameAsString() : f->getQualifiedNameAsString());

    auto cfun_or_cmethod = std::string{enforce_method and (not m) ? "cmethod" : "cfun"};
    if (m and parent_class and m->getParent() != parent_class) // it is a inherited method
      cfun_or_cmethod += "_B<" + parent_cls_name + '>';
    // if m is inherited, we add the <Cls> explicitly to pass Cls to the dispatcher properly

    logs.fun_c(fmt::format("{0}({1})", fname_log, fnt_param_with_types(f)));

    if (f_info.rewrite) {
      auto targs     = f->getTemplateSpecializationArgs() ? "<" + fnt_tparams(f) + ">" : "";
      auto call_name = m and parent_class and not m->isStatic() ?
         std::string{f->getTemplateSpecializationArgs() ? "self.template " : "self."} + f->getNameAsString() + targs :
         fname + targs;

      if (m and parent_class and not m->isStatic())
        return fmt::format(R"RAW( c2py::cmethod([]({0} {6} & self {1} {2}) -> decltype(auto) {{ return {3}({4}); }}, "self" {1} {5}))RAW", //
                           parent_cls_name, comma_if(args),                                                                                //
                           fnt_param_with_types(f), call_name, fnt_params(f), args, (m->isConst() ? "const" : ""));
      else
        return fmt::format(R"RAW( c2py::cfun([]({}) {{ return {}({}); }} {} {}))RAW", //
                           fnt_param_with_types(f), call_name, fnt_params(f), comma_if(args), args);

    } else {
      // almost never used except in user defined dispatch ? ...
      if (f->getTemplateSpecializationArgs())
        return fmt::format(R"RAW( c2py::{}( &{}<{}> {} {}))RAW", cfun_or_cmethod, fname, fnt_tparams(f), comma_if(args), args);
      else
        return fmt::format(R"RAW( c2py::{}( c2py::cast{}<{}>(&{}) {} {} ))RAW", cfun_or_cmethod,
                           (m ? (m->isStatic() ? "" : (m->isConst() ? "mc" : "m")) : ""), fnt_paramtypes(f), fname, comma_if(args), args);
      // static method -> cast, const method -> castmc, non const, non static method -> castm
    }
  };

  code << join(flist, l, ',') << " }";
  emit_deprecated_params(code, flist);
  code << ";\n";

  // ---- write the doc  ----
  // ORDERING INVARIANT: _c2py_doc_* must be emitted to the 'doc' stream (which the caller
  // flushes BEFORE the method/function table).
  auto [fdoc, param_types, return_types] = pydoc(flist);
  doc << '\n' << fmt::format(R"RAW( static const auto _c2py_doc_{0} = _c2py_fun_{0}.doc(R"DOC({1})DOC")RAW", fun_id, fdoc);
  if (not param_types.empty() or not return_types.empty()) {
    auto join_f = [](auto const &vec) { return fmt::format("{{{}}}", codegen::cpp_to_py_types(vec)); };
    doc << (param_types.empty() ? ", {}" : fmt::format(", {{{}}}", util::join(param_types, join_f, ", ")))
        << (return_types.empty() ? "" : fmt::format(", {{{}}}", codegen::cpp_to_py_types(return_types)));
  }
  doc << ");";

  // ---- put if in the table ----
  // the call function are special
  if (pyname == "__call__")
    code << '\n'
         << fmt::format(R"RAW(  template <> inline constexpr ternaryfunc c2py::tp_call<{0}> = c2py::pyfkw<_c2py_fun_{1}>;  )RAW", parent_cls_name,
                        fun_id)
         << '\n';
  else { // generic case
    // is one of the methods static ?
    auto is_static = std::any_of(flist.begin(), flist.end(), [](auto &fi) {
      auto *m = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(fi.ptr);
      return (m and m->isStatic());
    });

    // add in the table — PMDF macro lives in c2py/user_api.hpp
    // DIFF STABILITY: one row per line, with the indentation written here rather than by
    // clang-format. The tables are wrapped in clang-format off/on by the caller, because
    // clang-format would otherwise bin-pack these short rows into a column grid (and, if
    // forced one-per-line with a trailing comment, align the comments). In both cases the
    // padding depends on the other rows, so adding one function would reflow the whole
    // table. Emitting the final text verbatim makes it a pure one-line insertion.
    table << fmt::format(R"RAW(   PMDF("{}", {}{}),)RAW", pyname, fun_id, (is_static ? ", METH_STATIC" : "")) << '\n';
  }
}
// ===================================================================

void codegen::write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::string const &cls_log_name,
                                          std::vector<fnt_info_t> const &flist) {

  // One constructor dispatcher per class: hash of the class FQN.
  auto init_id = id_hash(cls_cpp_name);
  code << fmt::format(R"RAW( static const auto _c2py_init_{} = c2py::dispatcher_c_kw_t {{ )RAW", init_id) << '\n';

  logs.meth("__init__");

  auto l = [&cls_cpp_name, &cls_log_name](auto &f_info) {
    auto *f    = f_info.ptr;
    auto args  = fnt_params_with_default(f);
    auto targs = fnt_paramtypes(f);
    logs.fun_c(fmt::format("{0}({1})", cls_log_name, fnt_param_with_types(f)));

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
    logs.fun_c(fmt::format("{0}() [default]", cls_log_name));
  } else
    code << join(flist, l, ',');

  code << "}";
  emit_deprecated_params(code, flist);
  code << ";\n";

  code << fmt::format(R"RAW( template <> constexpr initproc c2py::tp_init<{}> = c2py::pyfkw_constructor<_c2py_init_{}>;)RAW", //
                      cls_cpp_name, init_id);

  // doc string for dispatched constructors
  auto [doc, param_types, return_types] = pydoc(flist);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_ctor_doc<{0}> = _c2py_init_{1}.doc(R"DOC({2})DOC")RAW", cls_cpp_name, init_id,
                      doc);
  if (not param_types.empty()) {
    auto join_f = [](auto const &vec) { return fmt::format("{{{}}}", codegen::cpp_to_py_types(vec)); };
    code << fmt::format(", {{{}}}", util::join(param_types, join_f, ", "));
  }
  code << ");";
}
