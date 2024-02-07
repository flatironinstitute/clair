#pragma once
#include "../data.hpp"

void codegen_cls(std::ostream &code, std::ostream &doc_stream, str_t const &cls_py_name, cls_info_t const &cls, str_t const &full_module_name);
