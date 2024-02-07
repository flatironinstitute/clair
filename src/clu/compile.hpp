#pragma once

#include <cassert>
#include <memory>
#include <string>

#include "clang/Frontend/CompilerInstance.h"

namespace clu {

  // Clone the compiler and make  a fresh compilation of code
  void compile(clang::CompilerInstance &ci, std::string const &code, std::string const &outfilename);
} // namespace clu
