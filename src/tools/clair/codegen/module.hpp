#pragma once
#include "../module_info.hpp"

str_t codegen_module(module_info_t const &m, bool add_forward_decls = false);

str_t codegen_wrap_info(module_info_t const &m);

str_t codegen_hxx(module_info_t const &m);
