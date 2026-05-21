#include <algorithm>
#include "fnt.hpp"
#include "./utils.hpp"
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include "utility/logger.hpp"
#include "utility/string_tools.hpp"
#include "./doc.hpp"

using util::join;

static const struct {
  util::logger fun   = util::logger{"-- ", "\033[1;32mFunction: \033[0m", 1};
  util::logger meth  = util::logger{"-- ", "\033[1;32m  Method: \033[0m", 1};
  util::logger fun_c = util::logger{"-- ", "\033[1;32m        . \033[0m", 2};
} logs;

// ===================================================================

// Collect C2PY_DEPRECATED_PARAMETER_NAME annotations from all overloads
// and emit .with_deprecated_params({...}) if any are found.
static void emit_deprecated_params(std::ostream &code, std::vector<fnt_info_t> const &flist) {
  std::map<str_t, str_t> deprecated_params;
  for (auto const &f_info : flist) {
    if (f_info.ptr and not f_info.ptr->deprecated_params_annotation.empty()) {
      for (auto &pair_str : util::split(f_info.ptr->deprecated_params_annotation, ',')) {
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
                             std::vector<fnt_info_t> const &flist, ir::RecordDecl const *ir_parent, bool enforce_method,
                             std::string const &cls_alias) {

  static long fun_counter = 0;

  if (ir_parent)
    logs.meth(fmt::format("{}", pyname));
  else
    logs.fun(fmt::format("{}", pyname));

  code << '\n'
       << fmt::format(R"RAW( // {}
                             static auto const _c2py_fun_{} = c2py::dispatcher_f_kw_t{{ )RAW",
                      pyname, fun_counter);

  // parent_cls_name: use the alias when available, else the FQN from IR.
  auto parent_cls_name = (not cls_alias.empty()) ? cls_alias : (ir_parent ? ir_parent->fully_qualified_name : str_t{});

  auto l = [&enforce_method, ir_parent, &parent_cls_name](fnt_info_t const &f_info) {
    auto &ir        = *f_info.ptr;
    bool has_parent = ir_parent != nullptr and ir.is_method;
    auto args       = fnt_params_with_default(ir);

    // The callable name at the call site
    auto fname = has_parent ? parent_cls_name + "::" + ir.simple_name
                            : (ir.is_inline_friend ? ir.simple_name : ir.qualified_name);

    auto fname_log = has_parent ? ir_parent->fully_qualified_name + "::" + ir.simple_name : ir.qualified_name;

    // cmethod vs cfun; _B<Cls> suffix when the method is inherited from a base class.
    auto cfun_or_cmethod = std::string{enforce_method and not ir.is_method ? "cmethod" : "cfun"};
    if (ir.is_method and has_parent and f_info.is_inherited_method)
      cfun_or_cmethod += "_B<" + parent_cls_name + '>';

    logs.fun_c(fmt::format("{0}({1})", fname_log, fnt_param_with_types(ir)));

    if (f_info.rewrite) {
      auto targs_suffix = ir.is_template_instantiation ? "<" + fnt_tparams(ir) + ">" : "";
      auto call_name    = has_parent and not ir.is_static
                            ? std::string{ir.is_template_instantiation ? "self.template " : "self."} + ir.simple_name + targs_suffix
                            : fname + targs_suffix;

      if (has_parent and not ir.is_static)
        return fmt::format(R"RAW( c2py::cmethod([]({0} {6} & self {1} {2}) -> decltype(auto) {{ return {3}({4}); }}, "self" {1} {5}))RAW",
                           parent_cls_name, comma_if(args), fnt_param_with_types(ir), call_name, fnt_params(ir), args,
                           (ir.is_const_method ? "const" : ""));
      else
        return fmt::format(R"RAW( c2py::cfun([]({}) {{ return {}({}); }} {} {}))RAW",
                           fnt_param_with_types(ir), call_name, fnt_params(ir), comma_if(args), args);
    } else {
      // Direct pointer cast (e.g. c2py_wrap_as_method, c2py_property_get/set free functions)
      if (ir.is_template_instantiation)
        return fmt::format(R"RAW( c2py::{}( &{}<{}> {} {}))RAW", cfun_or_cmethod, fname, fnt_tparams(ir), comma_if(args), args);
      else {
        auto cast_suffix = ir.is_method ? (ir.is_static ? "" : (ir.is_const_method ? "mc" : "m")) : "";
        return fmt::format(R"RAW( c2py::{}( c2py::cast{}<{}>(&{}) {} {} ))RAW", cfun_or_cmethod,
                           cast_suffix, fnt_paramtypes(ir), fname, comma_if(args), args);
      }
    }
  };

  code << join(flist, l, ',') << " }";
  emit_deprecated_params(code, flist);
  code << ";\n";

  // ---- write the doc ----
  // ORDERING INVARIANT: _c2py_doc_* must be emitted to the 'doc' stream (which the caller
  // flushes BEFORE the method/function table).
  auto [fdoc, param_types, return_types] = pydoc(flist);
  doc << '\n' << fmt::format(R"RAW( static const auto _c2py_doc_{0} = _c2py_fun_{0}.doc(R"DOC({1})DOC")RAW", fun_counter, fdoc);
  if (not param_types.empty() or not return_types.empty()) {
    auto join_f = [](auto const &vec) { return fmt::format("{{{}}}", codegen::cpp_to_py_types(vec)); };
    doc << (param_types.empty() ? ", {}" : fmt::format(", {{{}}}", util::join(param_types, join_f, ", ")))
        << (return_types.empty() ? "" : fmt::format(", {{{}}}", codegen::cpp_to_py_types(return_types)));
  }
  doc << ");";

  // ---- put it in the method table ----
  if (pyname == "__call__")
    code << '\n'
         << fmt::format(R"RAW(  template <> inline constexpr ternaryfunc c2py::tp_call<{0}> = c2py::pyfkw<_c2py_fun_{1}>;  )RAW", parent_cls_name,
                        fun_counter)
         << '\n';
  else {
    auto is_static = std::any_of(flist.begin(), flist.end(), [](auto &fi) {
      return fi.ptr and fi.ptr->is_method and fi.ptr->is_static;
    });
    table << fmt::format(R"RAW( {{"{}", (PyCFunction)c2py::pyfkw<_c2py_fun_{}>, METH_VARARGS | METH_KEYWORDS {}, _c2py_doc_{}.c_str()}}, )RAW",
                         pyname, fun_counter, (is_static ? "| METH_STATIC" : ""), fun_counter);
  }

  fun_counter++;
}

// ===================================================================

void codegen::write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::string const &cls_log_name,
                                          std::vector<fnt_info_t> const &flist) {

  static long counter = 0;
  code << fmt::format(R"RAW( static const auto _c2py_init_{} = c2py::dispatcher_c_kw_t {{ )RAW", counter) << '\n';

  logs.meth("__init__");

  auto l = [&cls_cpp_name, &cls_log_name](auto &f_info) {
    auto &ir   = *f_info.ptr;
    auto args  = fnt_params_with_default(ir);
    auto targs = fnt_paramtypes(ir);
    logs.fun_c(fmt::format("{0}({1})", cls_log_name, fnt_param_with_types(ir)));

    if (ir.is_constructor)
      return fmt::format(R"RAW( c2py::c_constructor<{}{}{}>({}))RAW", cls_cpp_name, comma_if(targs), targs, args);
    else {
      if (ir.is_template_instantiation)
        return fmt::format(R"RAW( c2py::c_maker<{}>( {}<{}> {} {}))RAW", cls_cpp_name, ir.qualified_name, fnt_tparams(ir), comma_if(args), args);
      else
        return fmt::format(R"RAW( c2py::c_maker<{}>( c2py::cast<{}>({}) {} {} ))RAW", cls_cpp_name, targs, ir.qualified_name, comma_if(args), args);
    }
  };

  if (flist.empty()) {
    code << fmt::format(R"RAW(c2py::c_constructor<{}>())RAW", cls_cpp_name);
    logs.fun_c(fmt::format("{0}() [default]", cls_log_name));
  } else
    code << join(flist, l, ',');

  code << "}";
  emit_deprecated_params(code, flist);
  code << ";\n";

  code << fmt::format(R"RAW( template <> constexpr initproc c2py::tp_init<{}> = c2py::pyfkw_constructor<_c2py_init_{}>;)RAW",
                      cls_cpp_name, counter);

  auto [doc, param_types, return_types] = pydoc(flist);
  code << '\n'
       << fmt::format(R"RAW(template <> const std::string c2py::tp_ctor_doc<{0}> = _c2py_init_{1}.doc(R"DOC({2})DOC")RAW", cls_cpp_name, counter, doc);
  if (not param_types.empty()) {
    auto join_f = [](auto const &vec) { return fmt::format("{{{}}}", codegen::cpp_to_py_types(vec)); };
    code << fmt::format(", {{{}}}", util::join(param_types, join_f, ", "));
  }
  code << ");";

  counter++;
}
