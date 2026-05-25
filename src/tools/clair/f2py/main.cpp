#include "utility/logger.hpp"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "flang/Frontend/CompilerInstance.h"
#include "flang/Frontend/FrontendAction.h"
#include "flang/Frontend/TextDiagnosticBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include <memory>
#include <stdexcept>

#include "custom_action.hpp"

//====================   main    ==========================================

int main(int argc, const char **argv) try {

  struct {
    util::logger error  = util::logger::error();
    util::logger report = util::logger{"-- ", "", 0};
  } const logs;

  // TODO: CMD line options, compilation database, etc.

  // ------- main tool
  /**
   * --- c2py --- 

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

   * --- c2py ---
   */

  std::unique_ptr<Fortran::frontend::CompilerInstance> flang = std::make_unique<Fortran::frontend::CompilerInstance>();

  flang->createDiagnostics();
  if (!flang->hasDiagnostics())
    return 1;

  auto diagsBuffer = std::make_unique<Fortran::frontend::TextDiagnosticBuffer>();

  clang::DiagnosticOptions diagOpts;
  clang::DiagnosticsEngine diags(clang::DiagnosticIDs::create(), diagOpts, diagsBuffer.get(), /*ShouldOwnClient=*/false);

  llvm::SmallVector<const char *, 256> args(argv, argv + argc);
  bool success = Fortran::frontend::CompilerInvocation::createFromArgs(
          flang->getInvocation(), llvm::ArrayRef(args).slice(1), diags, args[0]);
  if (!success)
    throw std::runtime_error("Failed.");

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();

  diagsBuffer->flushDiagnostics(flang->getDiagnostics());

  const std::unique_ptr<custom_action> act = std::make_unique<custom_action>();
  success = flang->executeAction(*act);

  flang->clearOutputFiles(false);

  if (!success)
    throw std::runtime_error("Failed.");
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
