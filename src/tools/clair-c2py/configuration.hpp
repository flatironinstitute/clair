#pragma once
#include "utility/string_tools.hpp"

struct configuration {

  str_t package_name  = {};
  str_t documentation = {};

  // filters
  str_t _namespace;
  str_t match_names  = {};
  str_t reject_names = {};
  str_t match_files  = {};

  bool has_module_init       = false;
  bool get_set_as_properties = false;
};

configuration configuration_from_toml(const std::string &toml_file);
