#include "./utils.hpp"

#include <algorithm>
#include <cctype>
#include <fmt/format.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include "utility/string_tools.hpp"

namespace codegen {

// ===========================================================================
// cpp_to_py_types
// ===========================================================================

std::string cpp_to_py_types(std::vector<std::string> const &cpp_types) {
  if (cpp_types.empty()) return std::string{};
  return fmt::format(R"RAW(c2py::python_typename<{}>())RAW", util::join(cpp_types, ">(), c2py::python_typename<"));
}

// ===========================================================================
// Fortran utilities — private helpers
// ===========================================================================

// Return the integer inside the outermost parens of s, or -1 if it is not a
// plain decimal integer (e.g. "INTEGER(4)" → 4, "CHARACTER(KIND=1,LEN=*)" → -1).
static int plain_kind(std::string_view s) {
  auto open  = s.find('(');
  auto close = s.find(')', open == std::string_view::npos ? 0 : open);
  if (open == std::string_view::npos || close == std::string_view::npos) return -1;
  auto inner = s.substr(open + 1, close - open - 1);
  for (char c : inner)
    if (!std::isdigit(static_cast<unsigned char>(c))) return -1;
  if (inner.empty()) return -1;
  try { return std::stoi(std::string(inner)); }
  catch (...) { return -1; }
}

// Extract the name between the outermost parens of "TYPE(name)" / "CLASS(name)"
// and lower-case it.  Returns an empty string on parse failure.
static std::string extract_derived_name(std::string_view s) {
  auto open  = s.find('(');
  auto close = s.rfind(')');
  if (open == std::string_view::npos || close == std::string_view::npos || close <= open) return {};
  std::string name{s.substr(open + 1, close - open - 1)};
  std::ranges::transform(name, name.begin(),
                         [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return name;
}

// Case-insensitive prefix test (Flang emits uppercase, but be defensive).
static bool starts_with_ci(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size()) return false;
  for (std::size_t i = 0; i < prefix.size(); ++i)
    if (std::toupper(static_cast<unsigned char>(s[i])) != std::toupper(static_cast<unsigned char>(prefix[i])))
      return false;
  return true;
}

// Split "ns1::ns2::Name" → {"ns1::ns2", "Name"}.  No namespace → first element is empty.
static std::pair<str_t, str_t> split_ns(str_t const &fqn) {
  auto pos = fqn.rfind("::");
  if (pos == str_t::npos) return {"", fqn};
  return {fqn.substr(0, pos), fqn.substr(pos + 2)};
}

// ===========================================================================
// fortran_type_to_cpp
// ===========================================================================

std::string fortran_type_to_cpp(std::string_view fort, std::string_view module_name) {
  if (starts_with_ci(fort, "INTEGER")) {
    switch (plain_kind(fort)) {
      case 1: return "int8_t";
      case 2: return "int16_t";
      case 4: return "int32_t";
      case 8: return "int64_t";
      default: return "int";
    }
  }
  if (starts_with_ci(fort, "REAL")) {
    switch (plain_kind(fort)) {
      case 4:  return "float";
      case 8:  return "double";
      case 16: return "long double";
      default: return "double";
    }
  }
  if (starts_with_ci(fort, "COMPLEX")) {
    switch (plain_kind(fort)) {
      case 4:  return "std::complex<float>";
      case 8:  return "std::complex<double>";
      case 16: return "std::complex<long double>";
      default: return "std::complex<double>";
    }
  }
  if (starts_with_ci(fort, "LOGICAL")) return "bool";
  if (starts_with_ci(fort, "CHARACTER")) return "const char *";
  if (starts_with_ci(fort, "TYPE(") || starts_with_ci(fort, "CLASS(")) {
    auto name = extract_derived_name(fort);
    if (name == "*") return "void *";
    if (!name.empty()) {
      if (!module_name.empty()) return std::string(module_name) + "::" + name;
      return name;
    }
  }
  return std::string(fort);
}

// ===========================================================================
// flang_procedure_name
// ===========================================================================

std::string flang_procedure_name(std::string_view module_name, std::string_view proc_name) {
  std::string result;
  result.reserve(4 + module_name.size() + proc_name.size());
  result = "_QM";
  for (unsigned char c : module_name) result += static_cast<char>(std::tolower(c));
  result += 'P';
  for (unsigned char c : proc_name) result += static_cast<char>(std::tolower(c));
  return result;
}

// ===========================================================================
// gen_forward_decls  (Fortran / f2py only — assumes linkage_name is set)
// ===========================================================================

std::string gen_forward_decls(module_info_t const &m) {
  std::vector<str_t>                  extern_c_lines;
  std::map<str_t, std::vector<str_t>> ns_decls;

  // Struct forward declarations
  for (auto const &[pyname, cls_info] : m.classes) {
    auto [ns, name] = split_ns(cls_info.ptr->fully_qualified_name);
    ns_decls[ns].push_back("struct " + name + ";");
  }

  // Free function declarations: extern "C" Flang symbol + inline namespace wrapper
  std::set<str_t> seen;
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
      ns_decls[ns].push_back(std::move(wrapper));
    }
  }

  // Emit
  std::stringstream out;
  if (!extern_c_lines.empty()) {
    out << "extern \"C\" {\n";
    for (auto const &l : extern_c_lines) out << "  " << l << "\n";
    out << "}\n";
  }
  for (auto const &[ns, decls] : ns_decls) {
    if (ns.empty()) {
      for (auto const &d : decls) out << d << "\n";
    } else {
      out << "namespace " << ns << " {\n";
      for (auto const &d : decls) out << "  " << d << "\n";
      out << "}\n";
    }
  }
  return out.str();
}

} // namespace codegen
