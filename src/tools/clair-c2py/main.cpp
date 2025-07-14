#include <filesystem>
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

  // ------- load the config if present

  fs::path input = opt_parser->getSourcePathList()[0];
  input.replace_extension(".toml");
  configuration config;
  if (fs::exists(input.string())) config = configuration_from_toml(input.string());

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

  //if (main_tool.run(new custom_action_factory{config})) //NOLINT new is ok here
  // to use multiple files, share the data in the factory
  if (main_tool.run(new custom_action_factory{})) //NOLINT new is ok here
    throw std::runtime_error("Failed.");

} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
