#include "llvm/Support/Process.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clu/cmd_line_arg.hpp"
#include "utility/macros.hpp"
#include "utility/logger.hpp"
#include "./action.hpp"

namespace cl = llvm::cl;

// ==========  Options of the program using LLVM ===================

static const cl::extrahelp OurHelp(R"HELPDOC(
  decorate_fnt matches all function in a namespace and add an annotation
  Usage e.g. : 
    ./decorate_fnt ess.cpp --annotation=__device__ -ns=nda -- -std=c++20
)HELPDOC");
static cl::OptionCategory decorate_fun_tool_category(""); //NOLINT
static const cl::opt<std::string> opt_ns("ns", cl::desc("Namespace to restrict the matching"), cl::cat(decorate_fun_tool_category));
static const cl::opt<std::string> opt_annotate("annotation", cl::desc("Annotation to add to the function"), cl::cat(decorate_fun_tool_category));

//====================   main    ==========================================

int main(int argc, const char **argv) try {

  struct {
    //util::logger error  = util::logger::error();
    util::logger report = util::logger{&std::cout, "-- ", ""};
  } const logs;

  // logs.report(fmt::format(R"RAW(Using clang version {}.{}.{})RAW", __clang_major__, __clang_minor__, __clang_patchlevel__));

  // ----- Parse the options in the command line
  auto opt_parser = clang::tooling::CommonOptionsParser::create(argc, argv, decorate_fun_tool_category);
  if (not opt_parser) {
    logs.report("Error in parsing the options. Use -help (or -h) to get documentation.");
    return EXIT_FAILURE;
  }

  auto config = config_t{opt_ns.c_str(), opt_annotate + " "};

  // ------- main tool

  clang::tooling::ClangTool main_tool(opt_parser->getCompilations(), opt_parser->getSourcePathList());

  // Additional Command line arguments to be given to the compiler, after all other options
  // from e.g. CXXFLAGS and co, and the -resource-dir.
  auto args = clu::get_clang_additional_args_from_env_variables();
  for (auto const &x : args) logs.report("Adding {}", x);
  main_tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(args, clang::tooling::ArgumentInsertPosition::END));

  if (main_tool.run(new custom_action_factory{config})) //NOLINT new is ok here
    throw std::runtime_error("Failed.");

} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
