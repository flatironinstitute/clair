#include "fortran.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace clu {

  // Return the integer inside the outermost parens of s, or -1 if it is not a
  // plain decimal integer (e.g. "INTEGER(4)" → 4, "CHARACTER(KIND=1,LEN=*)" → -1).
  static int plain_kind(std::string_view s) {
    auto open  = s.find('(');
    auto close = s.find(')', open == std::string_view::npos ? 0 : open);
    if (open == std::string_view::npos || close == std::string_view::npos) return -1;
    auto inner = s.substr(open + 1, close - open - 1);
    // Accept only pure digit strings — reject "KIND=1,LEN=*" etc.
    for (char c : inner)
      if (!std::isdigit(static_cast<unsigned char>(c))) return -1;
    if (inner.empty()) return -1;
    try { return std::stoi(std::string(inner)); }
    catch (...) { return -1; }
  }

  // Extract the name between the outermost parens of "TYPE(name)" / "CLASS(name)" and
  // lower-case it.  Returns an empty string on parse failure.
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

  // ---------------------------------------------------------------------------

  std::string fortran_type_to_cpp(std::string_view fort, std::string_view module_name) {
    // INTEGER(N) → fixed-width integer
    if (starts_with_ci(fort, "INTEGER")) {
      switch (plain_kind(fort)) {
        case 1: return "int8_t";
        case 2: return "int16_t";
        case 4: return "int32_t";
        case 8: return "int64_t";
        default: return "int"; // unrecognised kind → safe fallback
      }
    }

    // REAL(N)
    if (starts_with_ci(fort, "REAL")) {
      switch (plain_kind(fort)) {
        case 4:  return "float";
        case 8:  return "double";
        case 16: return "long double";
        default: return "double";
      }
    }

    // COMPLEX(N)
    if (starts_with_ci(fort, "COMPLEX")) {
      switch (plain_kind(fort)) {
        case 4:  return "std::complex<float>";
        case 8:  return "std::complex<double>";
        case 16: return "std::complex<long double>";
        default: return "std::complex<double>";
      }
    }

    // LOGICAL(N) — all kinds map to bool; Python cares about truthiness, not storage width
    if (starts_with_ci(fort, "LOGICAL")) return "bool";

    // CHARACTER — use const char * (Fortran C-interop convention)
    if (starts_with_ci(fort, "CHARACTER")) return "const char *";

    // TYPE(name) / CLASS(name)  — qualify with module name when available
    if (starts_with_ci(fort, "TYPE(") || starts_with_ci(fort, "CLASS(")) {
      auto name = extract_derived_name(fort);
      if (name == "*") return "void *"; // CLASS(*): unlimited polymorphic
      if (!name.empty()) {
        if (!module_name.empty()) return std::string(module_name) + "::" + name;
        return name;
      }
    }

    // Fallback: return verbatim so generated code at least compiles if Flang returns
    // something unexpected — the user will see an obvious type error at that point.
    return std::string(fort);
  }

} // namespace clu
