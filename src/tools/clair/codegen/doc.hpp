#pragma once
#include "../module_info.hpp"
#include "utility/string_tools.hpp"

#include <tuple>
#include <vector>

[[nodiscard]] std::tuple<str_t, std::vector<std::vector<str_t>>, std::vector<str_t>> pydoc(std::vector<fnt_info_t> const &f_list);
[[nodiscard]] str_t pydoc(cls_info_t const &cls);
[[nodiscard]] std::tuple<str_t, std::vector<str_t>> pydoc_of_synthetized_constructor(cls_info_t const &cls_info);
