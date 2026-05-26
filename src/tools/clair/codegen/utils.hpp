#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "../module_info.hpp"

namespace codegen {

  // Given a vector of C++ types, generate code that produces a vector of corresponding Python types.
  std::string cpp_to_py_types(std::vector<std::string> const &cpp_types);

  // ---------------------------------------------------------------------------
  // Fortran utilities
  // ---------------------------------------------------------------------------

  /// Convert a Fortran type string produced by DeclTypeSpec::AsFortran() to a C++ type name
  /// suitable for use as ir::QualType::name.
  ///
  ///   INTEGER(1/2/4/8) → int8_t / int16_t / int32_t / int64_t
  ///   REAL(4/8/16)     → float / double / long double
  ///   COMPLEX(4/8/16)  → std::complex<float/double/long double>
  ///   LOGICAL(*)       → bool
  ///   CHARACTER(*)     → const char *
  ///   TYPE(name) / CLASS(name) → module_name::name  (qualified when module_name is non-empty)
  ///   CLASS(*)         → void *
  ///   Anything else    → verbatim fallback
  std::string fortran_type_to_cpp(std::string_view fortran_type,
                                   std::string_view module_name = "");

  /// Compute the Flang linker name for a module-level procedure.
  /// Flang mangles "proc" in module "mod" as  _QM{mod}P{proc}  (all lowercase).
  std::string flang_procedure_name(std::string_view module_name,
                                    std::string_view proc_name);

  /// Generate the forward-declaration block for a Fortran module.
  ///
  /// Emits an  extern "C" { }  block with the Flang-mangled symbols, followed by
  /// namespace-grouped inline wrappers and struct forward declarations so that the
  /// rest of the generated wrapper can call  mymod::foo(a, b)  as usual:
  ///
  ///   extern "C" { float _QMmymodPadd(float *, float *); }
  ///   namespace mymod {
  ///     struct point_t;
  ///     inline float add(float _p0, float _p1) { return _QMmymodPadd(&_p0, &_p1); }
  ///   }
  ///
  /// NOTE: CHARACTER arguments use a complex ABI (hidden length argument); for
  /// correct interop add BIND(C) + VALUE to the Fortran declarations.
  std::string gen_forward_decls(module_info_t const &m);

} // namespace codegen
