#pragma once
#include "../clair-c2py/wdata.hpp"

str_t codegen_module(module_info_t const &m);

str_t codegen_wrap_info(module_info_t const &m);

str_t codegen_hxx(module_info_t const &m);
