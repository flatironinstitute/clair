#include "./forward_decls.hpp"

#include <map>
#include <set>
#include <sstream>
#include <string>

namespace codegen {

// ===========================================================================
// Private helpers
// ===========================================================================

// Split "ns1::ns2::Name" → {"ns1::ns2", "Name"}.  No namespace → first element is empty.
static std::pair<str_t, str_t> split_ns(str_t const &fqn) {
  auto pos = fqn.rfind("::");
  if (pos == str_t::npos) return {"", fqn};
  return {fqn.substr(0, pos), fqn.substr(pos + 2)};
}

// ===========================================================================
// gen_forward_decls  (Fortran / f2py only — assumes linkage_name is set)
// ===========================================================================

std::string gen_forward_decls(module_info_t const &m) {
  std::vector<str_t>                  extern_c_lines;
  // Forward declarations of types needed in extern "C" functions
  std::map<str_t, std::vector<str_t>> ns_fwd_decls;
  // Full definitions, including field and method forward declarations
  std::map<str_t, std::vector<str_t>> ns_full_decls;
  std::set<str_t>                     seen; // deduplicates across methods and free functions

  // ── Derived-type structs ──────────────────────────────────────────────────
  //
  // Non-static (passed-object) procedures live inside the struct so that the
  // generated lambda "self.method(args)" compiles.  Their body forwards to the
  // extern "C" Flang symbol collected into extern_c_lines.
  // NOPASS / static procedures are NOT added as struct members; the codegen
  // routes them through the free-function namespace wrappers below.
  for (auto const &[pyname, cls_info] : m.classes) {
    auto [ns, name] = split_ns(cls_info.ptr->fully_qualified_name);

    // Phase 1 — struct forward declaration
    ns_fwd_decls[ns].push_back("struct " + name + ";");

    // extern "C" declarations for non-static type-bound procedures
    for (auto const &[mname, overloads] : cls_info.methods) {
      for (auto const &fi : overloads) {
        if (!fi.ptr || fi.ptr->linkage_name.empty() || fi.ptr->is_static) continue;
        auto const &fd = *fi.ptr;
        // ABI: for TYPE(T) self → plain T*; for CLASS(T)/CLASS(*) self → Flang descriptor/box
        // (a pointer to a struct whose first field is the base-address pointer to the data).
        // Use void** to represent the box pointer in the extern "C" declaration.
        str_t ext_params = fd.self_is_polymorphic
            ? "void **"
            : cls_info.ptr->fully_qualified_name + " *";
        for (auto const &p : fd.params) {
          bool add_ptr = !p.is_fortran_value && p.type.name.find('*') == str_t::npos;
          ext_params += ", " + (add_ptr ? p.type.name + " *" : p.type.name);
        }
        auto decl_sig = fd.linkage_name + "(" + ext_params + ")";
        if (seen.insert(decl_sig).second)
          extern_c_lines.push_back(fd.return_type.name + " " + decl_sig + ";");
      }
    }

    // Phase 3 — full struct definition
    std::string def = "struct " + name + " {\n";

    // Data members
    for (auto const *field : cls_info.fields)
      def += "  " + field->type.name + " " + field->name + ";\n";

    // Non-static member functions, defined inline to call the extern "C" symbol.
    // When linkage_name is empty (e.g. pure virtual / abstract interface), a plain
    // declaration is emitted instead.
    for (auto const &[mname, overloads] : cls_info.methods) {
      for (auto const &fi : overloads) {
        if (!fi.ptr || fi.ptr->is_static) continue;
        auto const &fd = *fi.ptr;

        str_t member_params, call_args;
        for (std::size_t i = 0; i < fd.params.size(); ++i) {
          str_t sep   = (i > 0 ? ", " : "");
          str_t pname = fd.params[i].name.empty() ? "_p" + std::to_string(i) : fd.params[i].name;
          bool add_ptr = !fd.params[i].is_fortran_value && fd.params[i].type.name.find('*') == str_t::npos;
          member_params += sep + fd.params[i].type.name + " " + pname;
          call_args     += sep + (add_ptr ? "&" + pname : pname);
        }

        def += "  " + fd.return_type.name + " " + fd.simple_name
             + "(" + member_params + ")"
             + (fd.is_const_method ? " const" : "");

        if (!fd.linkage_name.empty()) {
          bool void_ret = (fd.return_type.name == "void");
          // Use :: prefix to call the global extern "C" symbol, avoiding any
          // accidental recursion if the binding name matches the wrapper name.
          if (fd.self_is_polymorphic) {
            // CLASS(T) ABI: build a minimal on-stack box whose first field is the
            // base-address pointer, then pass a pointer to the box.
            def += " { void *_base = this; ";
            if (!void_ret) def += "return ";
            def += "::" + fd.linkage_name + "(&_base"
                 + (call_args.empty() ? "" : ", " + call_args) + "); }";
          } else {
            def += " { ";
            if (!void_ret) def += "return ";
            def += "::" + fd.linkage_name + "(this"
                 + (call_args.empty() ? "" : ", " + call_args) + "); }";
          }
        } else {
          def += ";";
        }
        def += "\n";
      }
    }

    def += "};";
    ns_full_decls[ns].push_back(std::move(def));
  }

  // ── Free function declarations ────────────────────────────────────────────
  // extern "C" Flang symbol + inline namespace wrapper (phase 3)
  for (auto const &[pyname, overloads] : m.functions) {
    for (auto const &fi : overloads) {
      if (!fi.ptr) continue;
      auto const &fd  = *fi.ptr;
      auto sig = fd.qualified_name + "(" + fd.param_types_str() + ")";
      if (!seen.insert(sig).second) continue;
      auto [ns, name] = split_ns(fd.qualified_name);

      // Build parameter strings
      str_t ext_params, wrap_params, call_args;
      for (std::size_t i = 0; i < fd.params.size(); ++i) {
        auto const &p = fd.params[i];
        str_t sep   = (i > 0 ? ", " : "");
        str_t pname = "_p" + std::to_string(i);
        // Pointer types (e.g. "const char *") already carry indirection — don't add *.
        bool add_ptr = !p.is_fortran_value && p.type.name.find('*') == str_t::npos;
        ext_params  += sep + (add_ptr ? p.type.name + " *" : p.type.name);
        wrap_params += sep + p.type.name + " " + pname;
        call_args   += sep + (add_ptr ? "&" + pname : pname);
      }

      extern_c_lines.push_back(fd.return_type.name + " " + fd.linkage_name + "(" + ext_params + ");");

      bool void_ret = (fd.return_type.name == "void");
      str_t wrapper = "inline " + fd.return_type.name + " " + name + "(" + wrap_params + ") { ";
      if (!void_ret) wrapper += "return ";
      // Always use :: to avoid calling the wrapper recursively when the extern "C"
      // name matches the wrapper name (BIND(C) without an explicit NAME=).
      wrapper += "::" + fd.linkage_name + "(" + call_args + "); }";
      ns_full_decls[ns].push_back(std::move(wrapper));
    }
  }

  // ── Emit ─────────────────────────────────────────────────────────────────
  // Ordering is critical:
  //   1. Struct forward declarations  — makes "struct Foo *" legal in extern "C"
  //   2. extern "C" block             — Flang symbol prototypes
  //   3. Full struct definitions      — member bodies call the now-declared symbols
  auto emit_ns_map = [](std::stringstream &out,
                        std::map<str_t, std::vector<str_t>> const &nsmap) {
    for (auto const &[ns, decls] : nsmap) {
      if (ns.empty()) {
        for (auto const &d : decls) out << d << "\n";
      } else {
        out << "namespace " << ns << " {\n";
        for (auto const &d : decls) out << "  " << d << "\n";
        out << "}\n";
      }
    }
  };

  std::stringstream out;
  emit_ns_map(out, ns_fwd_decls);                   // phase 1
  if (!extern_c_lines.empty()) {                    // phase 2
    out << "extern \"C\" {\n";
    for (auto const &l : extern_c_lines) out << "  " << l << "\n";
    out << "}\n";
  }
  emit_ns_map(out, ns_full_decls);                  // phase 3
  return out.str();
}

} // namespace codegen
