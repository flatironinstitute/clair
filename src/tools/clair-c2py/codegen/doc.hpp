#pragma once
#include "../data.hpp"

[[nodiscard]] std::string pydoc(std::vector<fnt_info_t> const &f_list);
[[nodiscard]] std::string pydoc(cls_info_t const &cls);
