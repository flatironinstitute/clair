#pragma once
#include "tools/clair/module_info.hpp"

namespace Fortran::semantics {
  class Scope;
}

// Walk all publicly-accessible symbols in a Fortran MODULE scope and
// populate `mi` with functions, subroutines, and derived types.
void process_module_scope(Fortran::semantics::Scope const &modScope,
                           std::string const &module_name,
                           module_info_t &mi);
