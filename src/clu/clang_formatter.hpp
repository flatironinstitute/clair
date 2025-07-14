#pragma once
#include <iostream>
#include "clang/Format/Format.h"

namespace clu {

  // -------------------- clang-format a piece of code.
  /**
 * @brief 
 * 
 * @param code Code to format
 * @param style Style (typically taken from XXX )
 * @return std::string Formatted code 
 */
  std::string clang_format(std::string const &code, clang::format::FormatStyle style);

} // namespace clu
