#include "configuration.hpp"
#include <toml++/toml.h>
#include <fstream>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>

// Generic helper function to extract a value from a TOML table or throw an error if missing
template <typename T> T get_toml_value(const toml::table &table, const std::string &key) {
  if (auto value = table[key].value<T>()) return *value;
  throw std::runtime_error("Missing or invalid '" + key + "' in TOML file");
}
// ----------------------------------------

// Generic helper function to extract a value from a TOML table or use a default
template <typename T> T get_toml_value_or_default(const toml::table &table, const std::string &key, const T &default_value) {
  if (auto value = table[key].value<T>()) return *value;
  return default_value;
}
// ----------------------------------------

// Helper function to format error messages with context
std::string format_toml_error(const toml::parse_error &err) {
  std::ostringstream oss;
  oss << "Failed to parse TOML file: " << err.description() << "\n";

  const auto &source = err.source();
  oss << "At " << source.path << ":" << source.begin.line << ":" << source.begin.column << "\n";

  // Print a few lines around the error
  std::ifstream file(*source.path.get());
  if (file) {
    std::string line;
    for (long line_no = 1; std::getline(file, line); ++line_no) {
      if (std::abs(line_no - long(source.begin.line)) > 2) continue;
      oss << fmt::format("{}{}: {}\n", (line_no == source.begin.line ? ">> " : "   "), line_no, line);
      if (line_no == source.begin.line) oss << "   " << std::string(source.begin.column - 1, ' ') << "^\n";
    }
  }

  return oss.str();
}

// ----------------------------------------

configuration configuration_from_toml(const std::string &toml_file) try {
  configuration config;
  toml::table table = toml::parse_file(toml_file);

  // Extract required values (no default, must be present)

  // Extract optional values with defaults
  config.package_name          = get_toml_value_or_default<str_t>(table, "package_name", "");
  config.documentation         = get_toml_value_or_default<str_t>(table, "documentation", "");
  config._namespace            = get_toml_value_or_default<str_t>(table, "namespace", "");
  config.match_names           = get_toml_value_or_default<str_t>(table, "match_names", "");
  config.reject_names          = get_toml_value_or_default<str_t>(table, "reject_names", "");
  config.match_files           = get_toml_value_or_default<str_t>(table, "match_files", "");
  config.has_module_init       = get_toml_value_or_default<bool>(table, "has_module_init", false);
  config.get_set_as_properties = get_toml_value_or_default<bool>(table, "get_set_as_properties", false);

  // Check for spurious fields
  std::set<std::string> const valid_keys{"package_name", "documentation", "namespace",       "match_names",
                                         "reject_names", "match_files",   "has_module_init", "get_set_as_properties"};

  for (const auto &[key, value] : table) {
    if (valid_keys.find(std::string(key)) == valid_keys.end()) { throw std::runtime_error("Unknown field in TOML file: '" + std::string(key) + "'"); }
  }

  return config;
} catch (const toml::parse_error &err) {
  throw std::runtime_error(format_toml_error(err));
} //
catch (const std::exception &ex) {
  throw std::runtime_error("Error processing TOML file: " + std::string(ex.what()));
}