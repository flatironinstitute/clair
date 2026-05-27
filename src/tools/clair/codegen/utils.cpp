#include "./utils.hpp"

#include <algorithm>
#include <cctype>
#include <fmt/format.h>
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

} // namespace codegen
