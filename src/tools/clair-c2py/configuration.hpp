#pragma once

#include "utility/string_tools.hpp"

struct configuration {

  configuration(str_t const &module_filename) : match_files(module_filename), _module_filename{module_filename} {}

  str_t package_name  = {};
  str_t documentation = {};

  // filters
  str_t namespaces;
  str_t match_names  = {};
  str_t reject_names = {};
  str_t match_files  = {};

  bool wrap_no_arg_methods_as_properties = false;
  bool exclude_system_headers            = true;

  // Processed values
  std::vector<std::vector<str_t>> _namespaces_list; // list of namespaces, e.g. [ ["A"], ["A", "B"], ...] if namespace is "A A::B"
  str_t _module_filename;
  str_t _depfile_name; // name of the depfile, if any
};

configuration read_configuration(std::string const &toml_file_name);

void write_configuration(configuration const &config, const std::string &toml_file_name);
