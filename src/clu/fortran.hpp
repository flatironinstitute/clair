#pragma once
#include <string>
#include <string_view>

namespace clu {

/// Convert a Fortran type string produced by Fortran::semantics::DeclTypeSpec::AsFortran()
/// to a C++ type name suitable for use as ir::QualType::name in code generation.
///
/// Mapping:
///   INTEGER(1)   → int8_t          INTEGER(2)  → int16_t
///   INTEGER(4)   → int32_t         INTEGER(8)  → int64_t
///   REAL(4)      → float            REAL(8)     → double       REAL(16) → long double
///   COMPLEX(4)   → std::complex<float>          COMPLEX(8) → std::complex<double>
///   LOGICAL(*)   → bool  (all kinds; ABI is C-int for kind=4, but Python only cares about truthiness)
///   CHARACTER(*) → const char *     (assumed-length or fixed-length, C-interop convention)
///   TYPE(name) / CLASS(name) → module_name::name  (qualified when module_name is non-empty)
///   CLASS(*)     → void *           (unlimited polymorphic)
///   Anything else → returned verbatim as a safe fallback
///
/// @param fortran_type  The string from DeclTypeSpec::AsFortran(), e.g. "INTEGER(4)".
/// @param module_name   The enclosing Fortran module name used to qualify derived types.
///                      May be empty; in that case derived-type names are not qualified.
std::string fortran_type_to_cpp(std::string_view fortran_type,
                                 std::string_view module_name = "");

} // namespace clu
