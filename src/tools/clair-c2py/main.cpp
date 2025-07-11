#include "llvm/Support/Process.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clu/cmd_line_arg.hpp"
#include "utility/macros.hpp"
#include "utility/logger.hpp"
#include "action.hpp"

namespace cl = llvm::cl;

// ==========  Options of the program using LLVM ===================

static const cl::extrahelp OurHelp(R"HELPDOC(
  clang-c2py tool .....
  Usage e.g. : 
    clang-c2py my_module.cpp
)HELPDOC");
static cl::OptionCategory decorate_fun_tool_category(""); //NOLINT

//====================   main    ==========================================

int main(int argc, const char **argv) try {

  std::cerr << "Current working dir: " << std::filesystem::current_path() << "\n";

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

  // auto config = config_t{opt_ns.c_str(), opt_annotate + " "};

  // ------- main tool

  clang::tooling::ClangTool main_tool(opt_parser->getCompilations(), opt_parser->getSourcePathList());

  //   clang::tooling::ArgumentsAdjuster PrintAdjuster = [](const clang::tooling::CommandLineArguments &args, llvm::StringRef /*filename*/) {
  //     std::cerr << "Invoked with arguments:\n";
  //     for (const auto &arg : args) { std::cerr << arg << " "; }
  //     std::cerr << "\n---\n";
  //     return args;
  //   };

  //   main_tool.appendArgumentsAdjuster(PrintAdjuster);

  // Additional Command line arguments to be given to the compiler, after all other options
  // from e.g. CXXFLAGS and co, and the -resource-dir.
  auto args = clu::get_clang_additional_args_from_env_variables();
  args.emplace_back("-DCLAIR_WRAP_GEN");
  for (auto const &x : args) logs.report("Adding {}", x);
  main_tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(args, clang::tooling::ArgumentInsertPosition::END));

  std::cerr << "Current working dir: " << std::filesystem::current_path() << "\n";

  //if (main_tool.run(new custom_action_factory{config})) //NOLINT new is ok here
  if (main_tool.run(new custom_action_factory{})) //NOLINT new is ok here
    throw std::runtime_error("Failed.");

} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
