#include "interop.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include "flang/Evaluate/fold.h"
#include "flang/Semantics/type.h"

namespace flu {

namespace eval = Fortran::evaluate;
namespace sema = Fortran::semantics;
using     TC   = Fortran::common::TypeCategory;

// ---------------------------------------------------------------------------
// type_to_cpp
// ---------------------------------------------------------------------------

std::string type_to_cpp(sema::DeclTypeSpec const &t, std::string_view module_name) {
  // CLASS(*) / TYPE(*) — unlimited polymorphic or assumed-type → opaque pointer
  if (t.IsUnlimitedPolymorphic()) return "void *";

  // Intrinsic types: INTEGER, REAL, COMPLEX, LOGICAL, CHARACTER
  if (auto const *intr = t.AsIntrinsic()) {
    auto kind = eval::ToInt64(intr->kind()).value_or(-1);
    switch (intr->category()) {
      case TC::Integer:
        switch (kind) {
          case 1:  return "int8_t";
          case 2:  return "int16_t";
          case 4:  return "int32_t";
          case 8:  return "int64_t";
          default: return "int";
        }
      case TC::Real:
        switch (kind) {
          case 4:  return "float";
          case 8:  return "double";
          case 16: return "long double";
          default: return "double";
        }
      case TC::Complex:
        switch (kind) {
          case 4:  return "std::complex<float>";
          case 8:  return "std::complex<double>";
          case 16: return "std::complex<long double>";
          default: return "std::complex<double>";
        }
      case TC::Logical:   return "bool";
      case TC::Character: return "const char *";
      default: break;
    }
  }

  // Derived types: TYPE(T) or CLASS(T)
  if (auto const *derived = t.AsDerived()) {
    std::string name = derived->name().ToString();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!module_name.empty())
      return std::string(module_name) + "::" + name;
    return name;
  }

  // Fallback: use Fortran's own string representation
  return t.AsFortran();
}

// ---------------------------------------------------------------------------
// procedure_name
// ---------------------------------------------------------------------------

std::string procedure_name(std::string_view module_name, std::string_view proc_name) {
  std::string result;
  result.reserve(4 + module_name.size() + proc_name.size());
  result = "_QM";
  for (unsigned char c : module_name) result += static_cast<char>(std::tolower(c));
  result += 'P';
  for (unsigned char c : proc_name)   result += static_cast<char>(std::tolower(c));
  return result;
}

} // namespace flu
