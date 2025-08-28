#pragma once

#include <string>
#include <vector>

namespace codegen {

  // Given a vector of C++ types, generate code that produces a vector of corresponding Python types.
  std::string cpp_to_py_types(std::vector<std::string> const &cpp_types);

} // namespace codegen