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
#include "llvm/Support/Regex.h"
#include "llvm/ADT/StringRef.h"

#include "../../utility/logger.hpp"
static const struct {
  util::logger error = util::logger{&std::cout, "-- ", "\033[1;33mError:  \033[0m"};
} logs;

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
  config.package_name  = get_toml_value_or_default<str_t>(table, "package_name", "");
  config.documentation = get_toml_value_or_default<str_t>(table, "documentation", "");
  config.namespaces    = get_toml_value_or_default<str_t>(table, "namespace", "");
  config.match_names   = get_toml_value_or_default<str_t>(table, "match_names", "");
  config.reject_names  = get_toml_value_or_default<str_t>(table, "reject_names", "");
  config.match_files   = get_toml_value_or_default<str_t>(table, "match_files", "");

  // TO BE DISCUSSED
  config.get_set_as_properties = get_toml_value_or_default<bool>(table, "get_set_as_properties", false);

  // -------  Check that TOML entries are valid entries
  std::set<std::string> const valid_keys{"package_name", "documentation", "namespace",       "match_names",
                                         "reject_names", "match_files",   "has_module_init", "get_set_as_properties"};

  bool ok = true;
  for (const auto &[key, value] : table) {
    if (auto k = str_t{key}; not valid_keys.contains(k)) {
      ok = false;
      logs.error(fmt::format("Unknown field: {}", k));
    }
  }
  if (not ok) throw std::runtime_error("Too many errors. Aborting");

  // -------  Validation
  auto expect_regex = [](llvm::StringRef pattern, const char *name) {
    if (pattern.empty()) return;
    llvm::Regex R(pattern, llvm::Regex::NoFlags); // same flavour used by matchesName
    std::string ErrMsg;
    if (not R.isValid(ErrMsg))
      throw std::runtime_error(fmt::format("The key \033[1;31m{}\033[0m = \033[1m{}\033[0m  is not a valid regex", name, std::string{pattern}));
  };
  expect_regex(config.match_names, "match_names");
  expect_regex(config.reject_names, "reject_names");
  expect_regex(config.match_files, "match_files");

  // check the namespace is no regex, but a simple string
  if (not config.namespaces.empty()) {
    //static const std::regex ns_acceptable(R"(^[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*$)");
    //if (not std::regex_match(config._namespace, ns_acceptable))
    //  throw std::runtime_error(
    //     fmt::format("The key\033[1;31m namespace\033[0m = \033[1m{}\033[0m is not valid. It is NOT a regex, Cf doc.", config._namespace));
  }

  // all good !
  return config;
} catch (const toml::parse_error &err) {
  throw std::runtime_error(format_toml_error(err));
} //
catch (const std::exception &ex) {
  throw std::runtime_error("Error processing TOML file: " + toml_file + "\n" + std::string(ex.what()));
}