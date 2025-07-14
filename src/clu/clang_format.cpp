#include "./clang_format.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include "clang/Tooling/Tooling.h"

std::string clu::clang_format(std::string const &code, clang::format::FormatStyle style) {
  auto range        = std::vector<clang::tooling::Range>{{0, static_cast<unsigned int>(code.size())}};
  auto replacements = clang::format::reformat(style, code, range);
  auto result       = clang::tooling::applyAllReplacements(code, replacements);
  if (not result) { // just in case...
    auto error_log = util::logger{&std::cerr, "-- ", "\033[1;35mwarning: \033[0m"};
    error_log("Code can not be clang formatted. Formatting has failed.");
    return code;
  } else
    return *result;
}
