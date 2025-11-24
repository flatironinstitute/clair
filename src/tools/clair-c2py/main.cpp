#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "llvm/Support/Process.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Basic/Version.h"

#include "clu/custom_compilation_database.hpp"
#include "clu/cmd_line_arg.hpp"
#include "utility/macros.hpp"
#include "utility/logger.hpp"
#include "action.hpp"
#include "configuration.hpp"

namespace cl = llvm::cl;
namespace fs = std::filesystem;

// ==========  Options of the program using LLVM ===================

void printCustomVersion(llvm::raw_ostream &OS) {
  OS << "clair-c2py version (git hash) " << clu::get_git_hash() << "\n";
  OS << "  Based on " << clang::getClangToolFullVersion("clang") << "\n";
  OS.flush();
}
static const cl::extrahelp OurHelp(R"HELPDOC(
  clang-c2py generates Python binding for C++.
  Usage: 
   clair-c2py module_source_file.cpp -- all compiler options   # pass options on the command line, after the `--` separator
   clair-c2py module_source_file.cpp                           # uses compile_commands.json in the current directory
   clair-c2py module_source_file.cpp -p DIR                    # uses compile_commands.json from a specified directory DIR
)HELPDOC");
static cl::OptionCategory c2py_opt_category(""); //NOLINT
static const cl::opt<bool> opt_verbose("v", cl::desc("Verbose"), cl::cat(c2py_opt_category));
static const cl::opt<bool> opt_gen_default_config("gen-default-config", cl::desc("Generate a default TOML configuration file for each source file."),
                                                  cl::cat(c2py_opt_category));
static const cl::opt<bool> opt_update_config("update-config", cl::desc("Update the TOML configuration file for each source file."),
                                             cl::cat(c2py_opt_category));
static const cl::opt<str_t> opt_depfile("generate-depfile",
                                        cl::desc(R"RAW(Generates the depfile for CMake (encodes the dependencies of the bindings) )RAW"),
                                        cl::cat(c2py_opt_category));

//====================   main    ==========================================

int main(int argc, const char **argv) try {

  struct {
    util::logger error  = util::logger::error();
    util::logger report = util::logger{&std::cout, "-- ", ""};
  } const logs;

  // ----- Parse the options in the command line
  cl::SetVersionPrinter(printCustomVersion);
  auto opt_parser = clang::tooling::CommonOptionsParser::create(argc, argv, c2py_opt_category);
  if (not opt_parser) {
    logs.report("Error in parsing the options. Use -help (or -h) to get documentation.");
    return EXIT_FAILURE;
  }

  if (opt_verbose) logs.report(fmt::format(R"RAW(Based on clang version {}.{}.{})RAW", __clang_major__, __clang_minor__, __clang_patchlevel__));

  // ------- if the option --gen-default-config is present, we generate the config file and exit

  if (opt_gen_default_config or opt_update_config) {
    for (auto cpp_source : opt_parser->getSourcePathList()) {
      auto config_filename = fs::path{cpp_source}.replace_extension(".toml").string();
      configuration config = opt_update_config ? read_configuration(config_filename) : configuration{fs::path{cpp_source}.filename()};
      write_configuration(config, config_filename);
      logs.report(fmt::format("\033[1;34m{} configuration file {}\033[0m", (opt_update_config ? "Updated" : "Generated"), config_filename));
    }
    return EXIT_SUCCESS;
  }

  // ------- load the config if present else default
  // default config restricting to the module.cpp file (just filename, not full path)
  if (opt_parser->getSourcePathList().size() != 1) {
    logs.error(fmt::format("Expected exactly one source file, got {}.", opt_parser->getSourcePathList().size()));
    return EXIT_FAILURE;
  }
  str_t source0 = opt_parser->getSourcePathList()[0];
  configuration config{fs::path{source0}.filename()};
  if (auto config_filename = fs::path{source0}.replace_extension(".toml").string(); fs::exists(config_filename))
    config = read_configuration(config_filename);
  config._depfile_name = opt_depfile;

  // ---  Correct the compilation database
  // Enforce that the compiler for source0 is the clang compiler used in compiling clair itself.
  // This a ABSOLUTELY necessary to ensure clair sees the system paths, has the right -resource-dir.
  // It is useful in at least 2 cases:
  //  - call with -- options: the default "compiler" would be "clang-tool", and if the system path are not standard (i.e. in most cluster machine)
  //    it would fail.
  //  - when developing with another compiler, e.g. gcc.

  auto &current_compdb  = opt_parser->getCompilations();
  auto all_compile_cmds = current_compdb.getAllCompileCommands();
  // WARNING: Database behavior depends on the type of the compilation database:
  // - JSONCompilationDatabase (from compile_commands.json): getAllCompileCommands() returns all entries
  // - FixedCompilationDatabase (from -- arguments)        : getAllCompileCommands() returns empty always
  //   it can generate commands for any file on-demand via getCompileCommands(filename), so there's no finite "all" to return.
  // This seems to be a long-standing LLVM design choice [Cf Claude 4.5].
  // If future LLVM versions change this behavior, adjust the logic below accordingly.
  bool using_fixed_db = all_compile_cmds.empty();
  if (using_fixed_db) {
    llvm::errs() << "Calling with --. Getting compile commands for source file.\n";
    all_compile_cmds = current_compdb.getCompileCommands(source0);
    if (all_compile_cmds.empty())
      throw std::runtime_error("Internal Error: no compilation commands found or for " + source0 + "\n     and none could be generated.");
  }

  // Now fix the compiler for the compile command corresponding to source0.
  // Use canonical for comparison (to handle symlinks), but absolute for storage (ClangTool doesn't follow symlinks).
  auto canonical_source0 = fs::canonical(source0).string();
  for (auto &cmd : all_compile_cmds) {
    if (fs::canonical(cmd.Filename).string() == canonical_source0) {
      if (cmd.CommandLine.empty()) throw std::runtime_error("CompileCommand has empty CommandLine for " + source0);
      auto expected_compiler = clu::get_clang_compiler_path();
      if (cmd.CommandLine[0] != expected_compiler) {
        if (opt_verbose)
          llvm::errs() << "Warning: clair-c2py. When analyzing source file " << source0 << ", replacing compiler \n"
                       << cmd.CommandLine[0] << "\n   with \n"
                       << expected_compiler << "\n";
        cmd.CommandLine[0] = expected_compiler;
      }
      // For FixedCompilationDatabase: update Filename to canonical path.
      // For JSONCompilationDatabase: do nothing
      if (using_fixed_db) {
        cmd.Filename = canonical_source0;
        source0      = canonical_source0;
      }
      break; // we found and fixed the command for source0. we are done.
    }
  }
  // Finally we construct a custom compilation database with the fixed commands
  auto custom_db = clu::custom_compilation_database(all_compile_cmds);

  // ------- main tool
  // For FixedCompilationDatabase, pass canonical path to ClangTool to match what's in the database
  //auto source_for_tool = using_fixed_db ? canonical_source0 : source0;
  clang::tooling::ClangTool main_tool(custom_db, {source0}); // we use the fixed compilation database

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
