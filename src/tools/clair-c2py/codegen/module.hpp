#pragma once
#include "../data.hpp"

str_t codegen_module(module_info_t const &m);

str_t codegen_wrap_info(module_info_t const &m, bool write_header = false);

str_t codegen_hxx(module_info_t const &m);
