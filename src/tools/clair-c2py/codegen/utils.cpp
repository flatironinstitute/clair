#include "./utils.hpp"
#include "../../../utility/string_tools.hpp"

#include <fmt/format.h>

namespace codegen {

  std::string cpp_to_py_types(std::vector<std::string> const &cpp_types) {
    if (cpp_types.empty()) return std::string{};
    return fmt::format(R"RAW(c2py::python_typename<{}>())RAW", util::join(cpp_types, ">(), c2py::python_typename<"));
  };

} // namespace codegen