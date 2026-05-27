#include "./forward_decls.hpp"

#include <fmt/format.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include "utility/string_tools.hpp"

using util::join;

namespace codegen {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// Split "ns1::ns2::Name" → {"ns1::ns2", "Name"}.  No namespace → first element is empty.
static std::pair<str_t, str_t> split_ns(str_t const &fqn) {
  auto pos = fqn.rfind("::");
  if (pos == str_t::npos) return {"", fqn};
  return {fqn.substr(0, pos), fqn.substr(pos + 2)};
}

// Declaration ("T name") and call-site argument ("&name" or "name") for one parameter.
struct param_repr { str_t decl, call; };

// Build decl/call pairs for each parameter in a list.
// use_actual_names=true  → prefer declared names, fall back to _p0, _p1 … (methods)
// use_actual_names=false → always _p0, _p1 … (free-function wrappers)
static std::vector<param_repr> param_reprs(std::vector<ir::ParamVarDecl> const &params,
                                            bool use_actual_names = true) {
  std::vector<param_repr> result;
  result.reserve(params.size());
  for (std::size_t i = 0; i < params.size(); ++i) {
    auto const &p = params[i];
    str_t pname   = (use_actual_names && !p.name.empty()) ? p.name : "_p" + std::to_string(i);
    bool  by_ptr  = !p.is_fortran_value && p.type.name.find('*') == str_t::npos;
    result.push_back({.decl = p.type.name + " " + pname, .call = by_ptr ? "&" + pname : pname});
  }
  return result;
}

// Build the extern "C" parameter-type list (no names; pass-by-ref params become T*).
static str_t extern_c_params(std::vector<ir::ParamVarDecl> const &params) {
  return join(params, [](auto const &p) -> str_t {
    return (!p.is_fortran_value && p.type.name.find('*') == str_t::npos)
        ? p.type.name + " *" : p.type.name;
  }, ", ");
}

// Build the inline struct method that forwards to the extern "C" Flang symbol.
static str_t inline_method(ir::FunctionDecl const &fd) {
  auto prs    = param_reprs(fd.params);
  auto decls  = join(prs, [](auto const &p) { return p.decl; }, ", ");
  auto args   = join(prs, [](auto const &p) { return p.call; }, ", ");
  str_t cv      = fd.is_const_method ? " const" : "";
  str_t ret     = fd.return_type.name == "void" ? "" : "return ";
  str_t trailer = args.empty() ? "" : ", " + args;

  if (fd.self_is_polymorphic)
    return fmt::format("  {} {}({}){} {{ void *_base = this; {}::{}(&_base{}); }}",
                       fd.return_type.name, fd.simple_name, decls, cv, ret, fd.linkage_name, trailer);
  else
    return fmt::format("  {} {}({}){} {{ {}::{}(this{}); }}",
                       fd.return_type.name, fd.simple_name, decls, cv, ret, fd.linkage_name, trailer);
}

// Emit a map of namespace → declarations, wrapping each non-empty namespace.
static void emit_ns_map(std::stringstream &out, std::map<str_t, std::vector<str_t>> const &nsmap) {
  for (auto const &[ns, decls] : nsmap) {
    if (ns.empty()) {
      for (auto const &d : decls) out << d << "\n";
    } else {
      out << "namespace " << ns << " {\n";
      for (auto const &d : decls) out << "  " << d << "\n";
      out << "}\n";
    }
  }
}

// ---------------------------------------------------------------------------
// gen_forward_decls  (Fortran / f2py only — assumes linkage_name is set)
// ---------------------------------------------------------------------------

std::string gen_forward_decls(module_info_t const &m) {
  std::vector<str_t>                  extern_c_lines;
  std::map<str_t, std::vector<str_t>> ns_fwd_decls;
  std::map<str_t, std::vector<str_t>> ns_full_decls;
  std::set<str_t>                     seen; // deduplicates across methods and free functions

  // ── Derived-type structs ──────────────────────────────────────────────────
  //
  // Non-static (passed-object) methods live inside the struct so the generated
  // lambda "self.method(args)" compiles; their body forwards to the extern "C"
  // Flang symbol.  NOPASS/static procedures go through the free-function wrappers.
  for (auto const &[pyname, cls_info] : m.classes) {
    auto [ns, name] = split_ns(cls_info.ptr->fully_qualified_name);

    // Phase 1 — struct forward declaration
    ns_fwd_decls[ns].push_back(fmt::format("struct {};", name));

    // Phase 2 — extern "C" declarations for non-static type-bound procedures
    for (auto const &[mname, overloads] : cls_info.methods) {
      for (auto const &fi : overloads) {
        if (!fi.ptr || fi.ptr->linkage_name.empty() || fi.ptr->is_static) continue;
        auto const &fd = *fi.ptr;
        // TYPE(T) self → plain T*; CLASS(T)/CLASS(*) → Flang descriptor/box (void**).
        str_t self = fd.self_is_polymorphic ? "void **" : cls_info.ptr->fully_qualified_name + " *";
        auto  ext  = extern_c_params(fd.params);
        auto  sig  = fmt::format("{}({}{}{});", fd.linkage_name, self, ext.empty() ? "" : ", ", ext);
        if (seen.insert(sig).second)
          extern_c_lines.push_back(fd.return_type.name + " " + sig);
      }
    }

    // Phase 3 — full struct definition with fields and inline forwarding methods
    auto fields = join(cls_info.fields, [](auto const *f) {
      return fmt::format("  {} {};", f->type.name, f->name);
    }, "\n");

    std::string methods;
    for (auto const &[mname, overloads] : cls_info.methods) {
      for (auto const &fi : overloads) {
        if (!fi.ptr || fi.ptr->is_static) continue;
        auto const &fd = *fi.ptr;
        if (!fd.linkage_name.empty()) {
          methods += inline_method(fd) + "\n";
        } else {
          // No linkage name (e.g. abstract interface) → plain declaration only.
          auto prs = param_reprs(fd.params);
          methods += fmt::format("  {} {}({}){};\n", fd.return_type.name, fd.simple_name,
                                 join(prs, [](auto const &p) { return p.decl; }, ", "),
                                 fd.is_const_method ? " const" : "");
        }
      }
    }

    ns_full_decls[ns].push_back(fmt::format("struct {} {{\n{}{}}};",
        name, fields.empty() ? "" : fields + "\n", methods));
  }

  // ── Free function declarations ────────────────────────────────────────────
  for (auto const &[pyname, overloads] : m.functions) {
    for (auto const &fi : overloads) {
      if (!fi.ptr) continue;
      auto const &fd = *fi.ptr;
      auto sig = fd.qualified_name + "(" + fd.param_types_str() + ")";
      if (!seen.insert(sig).second) continue;
      auto [ns, name] = split_ns(fd.qualified_name);

      auto prs   = param_reprs(fd.params, /*use_actual_names=*/false);
      auto decls = join(prs, [](auto const &p) { return p.decl; }, ", ");
      auto args  = join(prs, [](auto const &p) { return p.call; }, ", ");
      str_t ret  = fd.return_type.name == "void" ? "" : "return ";

      // Always use :: to avoid calling the wrapper recursively when the extern "C"
      // name matches the wrapper name (BIND(C) without an explicit NAME=).
      extern_c_lines.push_back(fmt::format("{} {}({});",
          fd.return_type.name, fd.linkage_name, extern_c_params(fd.params)));
      ns_full_decls[ns].push_back(fmt::format("inline {} {}({}) {{ {}::{}({}); }}",
          fd.return_type.name, name, decls, ret, fd.linkage_name, args));
    }
  }

  // ── Emit ─────────────────────────────────────────────────────────────────
  // Ordering is critical:
  //   1. Struct forward declarations  — makes "struct Foo *" legal in extern "C"
  //   2. extern "C" block             — Flang symbol prototypes
  //   3. Full struct definitions      — member bodies call the now-declared symbols
  std::stringstream out;
  emit_ns_map(out, ns_fwd_decls);
  if (!extern_c_lines.empty()) {
    out << "extern \"C\" {\n";
    for (auto const &l : extern_c_lines) out << "  " << l << "\n";
    out << "}\n";
  }
  emit_ns_map(out, ns_full_decls);
  return out.str();
}

} // namespace codegen
