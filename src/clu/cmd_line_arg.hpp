#pragma once
#include "llvm/Support/CommandLine.h"
#include "clang/Tooling/Tooling.h"

namespace clu {

  // For TOOLS only, not plugins
  // Build additional command line argument from the environment variables
  // - CXXFLAGS
  // - CPATH
  // - CPLUS_INCLUDE_PATH
  // and add the -resource-dir argument as detected from cmake.
  // We MUST fix the resource_dir on OS X in particular, or the
  // include e.g. on stdlib will fail
  clang::tooling::CommandLineArguments get_clang_additional_args_from_env_variables(bool is_verbose = true);
} // namespace clu
