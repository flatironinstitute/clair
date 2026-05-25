#include "wdata.hpp"

wdata_t::wdata_t(Fortran::frontend::CompilerInstance *ci) : ci{ci} {}

module_info_t const *wdata_t::intern(module_info_t module_info) {
  modules.push_back(std::make_unique<module_info_t>(std::move(module_info)));
  return modules.back().get();
}
