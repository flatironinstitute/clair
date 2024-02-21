#include "cmd_line_arg.hpp"
#include <iostream>
#include "llvm/Support/Process.h"
#include "utility/string_tools.hpp"

#include <filesystem>
namespace fs = std::filesystem;

// a trick to silence clangd ... the macro is always defined by cmake ...
#ifndef CLANG_RESOURCE_DIR
const auto CLANG_RESOURCE_DIR = "";
#endif
static const std::string resource_dir = CLANG_RESOURCE_DIR;

namespace clu {
  clang::tooling::CommandLineArguments get_clang_additional_args_from_env_variables() {

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
} // namespace clu
