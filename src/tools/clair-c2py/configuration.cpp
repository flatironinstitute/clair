#include "configuration.hpp"
#include <set>
#include <filesystem>
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <toml++/toml.h>
#include "llvm/Support/Regex.h"
#include "llvm/ADT/StringRef.h"

#include "../../utility/logger.hpp"
#include "../../utility/string_tools.hpp"

namespace fs = std::filesystem;
static const struct {
  util::logger error = util::logger{&std::cout, "-- ", "\033[1;31mError:  \033[0m"};
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

  const auto &toml_src = err.source();
  oss << "At " << toml_src.path << ":" << toml_src.begin.line << ":" << toml_src.begin.column << "\n";

  // Print a few lines around the error
  std::ifstream file(*toml_src.path.get());
  if (file) {
    std::string line;
    for (long line_no = 1; std::getline(file, line); ++line_no) {
      if (std::abs(line_no - long(toml_src.begin.line)) > 2) continue;
      oss << fmt::format("{}{}: {}\n", (line_no == toml_src.begin.line ? ">> " : "   "), line_no, line);
      if (line_no == toml_src.begin.line) oss << "   " << std::string(toml_src.begin.column - 1, ' ') << "^\n";
      // ahah
    }
  }

  return oss.str();
}

// ----------------------------------------

configuration read_configuration(std::string const &toml_file_name) try {
  configuration config{""}; // nothing
  toml::table table = toml::parse_file(toml_file_name);

  // Extract required values (no default, must be present)

  // Extract optional values with defaults
  config.package_name  = util::trim(get_toml_value_or_default<str_t>(table, "package_name", ""));
  config.documentation = get_toml_value_or_default<str_t>(table, "documentation", ""); // no trim
  config.namespaces    = util::trim(get_toml_value_or_default<str_t>(table, "namespaces", ""));
  config.match_names   = util::trim(get_toml_value_or_default<str_t>(table, "match_names", ""));
  config.reject_names  = util::trim(get_toml_value_or_default<str_t>(table, "reject_names", ""));
  config.match_files   = util::trim(get_toml_value_or_default<str_t>(table, "match_files", ""));

  // TO BE DISCUSSED
  config.wrap_no_arg_methods_as_properties = get_toml_value_or_default<bool>(table, "wrap_no_arg_methods_as_properties", false);
  config.exclude_system_headers            = get_toml_value_or_default<bool>(table, "exclude_system_headers", true);

  // -------  Check that TOML entries are valid entries
  std::set<std::string> const valid_keys{"package_name",          "documentation", "namespaces",      "match_names",
                                         "reject_names",          "match_files",   "has_module_init", "wrap_no_arg_methods_as_properties",
                                         "exclude_system_headers"};

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

  // We transform and validate the namespaces string
  // _namespaces_list is list [ ["A"], ["A", "B"], ...] if namespace is "A A::B"
  for (const auto &nsPath : util::split(config.namespaces)) {
    if (util::trim(nsPath).empty()) continue; // skip empty namespaces
    static const llvm::Regex ns_acceptable(R"(^[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*$)");
    if (not ns_acceptable.match(nsPath)) {
      throw std::runtime_error(
         fmt::format("The key \033[1;31mnamespace\033[0m = \033[1m{}\033[0m is not valid. It is not a list of namespaces", nsPath));
    }
    // Split "A::B::C" into {"A", "B", "C"}
    std::vector<str_t> parts;
    std::stringstream ss(nsPath);
    std::string part;
    while (std::getline(ss, part, ':')) {
      if (!part.empty() && part != ":") parts.push_back(part);
      if (ss.peek() == ':') ss.get(); // consume second ':'
    }
    config._namespaces_list.push_back(std::move(parts));
  }

  // We could put a warning, but it will likely to produce a lot of wrapping, maybe some very corner cases in boost or alike
  // that could crash the tool (?) and be very confusing for the user...
  // Better to throw an error
  if (config.match_names.empty() and config.reject_names.empty() and config.namespaces.empty() and config.match_files.empty())
    throw std::runtime_error("The key \033[1;31mIncorrect configuration: match_name, reject_names, namespaces and match_files are all empty\033[0m");

  return config;
} catch (const toml::parse_error &err) {
  throw std::runtime_error(format_toml_error(err));
} //
catch (const std::exception &ex) {
  throw std::runtime_error("Error processing TOML file: " + toml_file_name + "\n" + std::string(ex.what()));
}

//--------------------------------------------------

void write_configuration(configuration const &config, std::string const &toml_file_name) {
// #embed is C, it will be C++23, meanwhile we silence the warning that we use a C extension
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
  static constexpr char config_default[] = {//NOLINT
#embed "configuration.toml.template"
                                            , '\0'};
#pragma clang diagnostic pop

  // if the string contains a newline, we transform it into a multiline string for toml
  auto l = [](const std::string &s) -> std::string { return (s.find('\n') != std::string::npos) ? "\"\"" + s + "\"\"" : s; };

  std::ofstream{toml_file_name} << fmt::format(config_default, config.package_name, l(config.documentation), config.namespaces, config.match_names,
                                               config.reject_names, config.match_files, config.wrap_no_arg_methods_as_properties,
                                               config.exclude_system_headers);
}
