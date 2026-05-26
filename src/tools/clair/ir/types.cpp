#include "types.hpp"

#include <fmt/format.h>
#include "utility/string_tools.hpp"

namespace ir {

  str_t FunctionDecl::param_names_str() const {
    return util::join(params, [](auto const &p) { return p.name; }, ',');
  }

  str_t FunctionDecl::param_types_str() const {
    return util::join(params, [](auto const &p) { return p.type.name; }, ',');
  }

  str_t FunctionDecl::params_with_types_str() const {
    return util::join(params, [](auto const &p) { return p.type.name + ' ' + p.name; }, ',');
  }

  str_t FunctionDecl::params_with_defaults_str() const {
    return util::join(params, [](auto const &p) {
      auto res = fmt::format(R"RAW( "{}")RAW", p.name);
      if (p.has_default) res += "_a = " + p.default_val;
      return res;
    }, ',');
  }

  str_t FunctionDecl::targs_str() const {
    return util::join(targs, [](auto const &s) { return s; }, ',');
  }

} // namespace ir
