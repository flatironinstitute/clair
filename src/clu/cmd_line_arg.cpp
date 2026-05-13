#include "cmd_line_arg.hpp"
#include <iostream>
#include "llvm/Support/Process.h"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"

#include <filesystem>
#include <stdexcept>
namespace fs = std::filesystem;
using namespace std::string_literals;

static const std::string resource_dir = CLANG_RESOURCE_DIR;
static const std::string cxx_compiler = CMAKE_CXX_COMPILER;

namespace clu {
  std::string get_clang_compiler_path() { return cxx_compiler; }

  clang::tooling::CommandLineArguments get_clang_additional_args_from_env_variables() {

#ifdef __APPLE__
    // We examine the SDKROOT and set it for this process is the machine is poorly configured.
    const char *sdkroot = std::getenv("SDKROOT");
    if (!sdkroot || std::string(sdkroot).empty()) {
      setenv("SDKROOT", SDKROOT, 1);                                      // overwrite = 1, for this process only
      util::logger{"-- ", "", 1}("SDKROOT set to: "s + SDKROOT); // report !
    } else {
      if (sdkroot != std::string{SDKROOT}) // SDKROOT is set but not to the expected value. That is strange.
        util::logger::warning()("\033[1;31m SDKROOT inconsistent between xcrun --show-sdk-path and the environement variable \033[0m");
    }
#endif

    clang::tooling::CommandLineArguments arguments;

    // // Add the ressource dir of the compiler
    //fs::path resource_dir{resource_dir}; // NOLINT cmake detected MACRO

    assert(fs::exists(resource_dir));
    arguments.emplace_back("-resource-dir=" + resource_dir);

    // Use CXXFLAGS
    if (auto cxxflags = llvm::sys::Process::GetEnv("CXXFLAGS"))
      for (auto &&x : util::split(*cxxflags)) arguments.emplace_back(x);

    // Use CPATH
    if (auto cpath = llvm::sys::Process::GetEnv("CPATH"))
      for (auto &&x : util::split(*cpath, ':'))
        if (!util::trim(x).empty()) arguments.emplace_back("-I" + x);

    // Use CPLUS_INCLUDE_PATH
    if (auto cplus_include_path = llvm::sys::Process::GetEnv("CPLUS_INCLUDE_PATH"))
      for (auto &&x : util::split(*cplus_include_path, ':'))
        if (!util::trim(x).empty()) arguments.emplace_back("-isystem" + x);

    return arguments;
  }

  //==============================

  std::string get_git_hash() { return GIT_HASH; }
} // namespace clu
