#pragma once
#include "utility/string_tools.hpp"

namespace Fortran::semantics { class Symbol; }

/// Return the Python name for a Fortran derived-type symbol.
/// Applies camelCase conversion: "t_square" → "TSquare".
str_t get_python_name_cls(Fortran::semantics::Symbol const &sym);
