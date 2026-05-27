#include "decl_utils.hpp"

#include "flang/Semantics/symbol.h"
#include "utility/string_tools.hpp"

namespace sema = Fortran::semantics;

str_t get_python_name_cls(sema::Symbol const &sym) {
  return util::camel_case(sym.name().ToString());
}
