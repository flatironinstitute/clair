#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "llvm/Support/Process.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clu/cmd_line_arg.hpp"
#include "utility/macros.hpp"
#include "utility/logger.hpp"
#include "action.hpp"
#include "configuration.hpp"

namespace cl = llvm::cl;
namespace fs = std::filesystem;

// ==========  Options of the program using LLVM ===================

static const cl::extrahelp OurHelp(R"HELPDOC(
  clang-c2py generates Python binding for C++.
  Usage: 
    clang-c2py my_module.cpp
)HELPDOC");
static cl::OptionCategory c2py_opt_category(""); //NOLINT
static const cl::opt<bool> opt_verbose("v", cl::desc("Verbose"), cl::cat(c2py_opt_category));
static const cl::opt<bool> opt_gen_default_config("gen-default-config", cl::desc("Generate a default TOML configuration file for each source file."),
                                                  cl::cat(c2py_opt_category));

//====================   main    ==========================================

int main(int argc, const char **argv) try {

  struct {
    util::logger error  = util::logger::error();
    util::logger report = util::logger{&std::cout, "-- ", ""};
  } const logs;

  // ----- Parse the options in the command line
  auto opt_parser = clang::tooling::CommonOptionsParser::create(argc, argv, c2py_opt_category);
  if (not opt_parser) {
    logs.report("Error in parsing the options. Use -help (or -h) to get documentation.");
    return EXIT_FAILURE;
  }

  if (opt_verbose) logs.report(fmt::format(R"RAW(Based on clang version {}.{}.{})RAW", __clang_major__, __clang_minor__, __clang_patchlevel__));

  // ------- if the option --gen-default-config is present, we generate the config file and exit

  if (opt_gen_default_config) {
    for (auto cpp_source : opt_parser->getSourcePathList()) {
      auto config_filename = fs::path{cpp_source}.replace_extension(".toml").string();
// #embed is C, it will be C++23, meanwhile we silence the warning that we use a C extension
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
      constexpr char config_default[] = { //NOLINT
#embed "configuration.default.toml"
         , '\0'};
#pragma clang diagnostic pop
      std::ofstream{config_filename} << config_default;
      logs.report(fmt::format("\033[1;34mGenerated configuration file {}\033[0m", config_filename));
      return EXIT_SUCCESS;
    }
  }

  // ------- load the config if presen

  auto config_filename = fs::path{opt_parser->getSourcePathList()[0]}.replace_extension(".toml").string();
  configuration config = {};
  // load the config from the file if present, else we keept the default config, i.e. equivalent to an empty file.
  if (fs::exists(config_filename)) config = configuration_from_toml(config_filename);

  // ------- main tool

  clang::tooling::ClangTool main_tool(opt_parser->getCompilations(), opt_parser->getSourcePathList());

  // Additional Command line arguments to be given to the compiler, after all other options
  // from e.g. CXXFLAGS and co, and the -resource-dir.
  auto args = clu::get_clang_additional_args_from_env_variables();
  args.emplace_back("-DCLAIR_C2PY_WRAP_GEN");
  // DEBUG ONLY
  //args.emplace_back("-DCLAIR_WRAP_GEN");
  //args.emplace_back("-Wno-unused-const-variable");
  //args.emplace_back("-Wno-unused-variable");
  if (opt_verbose)
    for (auto const &x : args) logs.report("Adding {}", x);
  main_tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(args, clang::tooling::ArgumentInsertPosition::END));

  // to use multiple files, share the data in the factory
  if (main_tool.run(new custom_action_factory{config})) //NOLINT new is ok here
    throw std::runtime_error("Failed.");
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
