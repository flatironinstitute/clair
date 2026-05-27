#pragma once
#include <string>
#include <string_view>

// Forward declaration — avoids pulling heavy Flang headers into every TU that needs flu.
namespace Fortran::semantics { class DeclTypeSpec; }

namespace flu {

  /// Map a Fortran DeclTypeSpec to the corresponding C++ type name.
  ///
  ///   INTEGER(1/2/4/8)  → int8_t / int16_t / int32_t / int64_t
  ///   REAL(4/8/16)      → float / double / long double
  ///   COMPLEX(4/8/16)   → std::complex<float/double/long double>
  ///   LOGICAL           → bool
  ///   CHARACTER         → const char *
  ///   TYPE(T)/CLASS(T)  → module_name::t  (qualified when module_name is non-empty)
  ///   CLASS(*)/TYPE(*)  → void *
  ///   Anything else     → DeclTypeSpec::AsFortran() verbatim fallback
  std::string type_to_cpp(Fortran::semantics::DeclTypeSpec const &,
                           std::string_view module_name = "");

  /// Compute the Flang linker name for a module-level procedure.
  /// Flang mangles "proc" in module "mod" as  _QM{mod}P{proc}  (all lowercase).
  std::string procedure_name(std::string_view module_name, std::string_view proc_name);

} // namespace flu
