#pragma once
#include "utility/string_tools.hpp"

struct configuration {

  str_t package_name  = {};
  str_t documentation = {};

  // filters
  str_t namespaces;
  str_t match_names  = {};
  str_t reject_names = {};
  str_t match_files  = {};

  bool wrap_no_arg_methods_as_properties = false;

  // Processed values
  std::vector<std::vector<str_t>> _namespaces_list; // list of namespaces, e.g. [ ["A"], ["A", "B"], ...] if namespace is "A A::B"
};

configuration configuration_from_toml(const std::string &toml_file);

str_t write_configuration(configuration const &config, const std::string &cpp_source);
