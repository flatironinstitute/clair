#include "./custom_action.hpp"

#include <fstream>
#include <fmt/core.h>
#include <fmt/format.h>

#include "clang/Rewrite/Core/Rewriter.h"
#include "clu/clang_format.hpp"
#include "utility/logger.hpp"
#include "utility/stl_complement.hpp"

#include "./ast_consumer.hpp"
#include "./codegen/module.hpp"
#include "./pp_include_callback.hpp"

// ------------------------------

bool custom_action::BeginInvocation(clang::CompilerInstance &CI) {
  // Skip function bodies, it gains parsing time, and we do not need them.
  // CI.getInvocation().getFrontendOpts().SkipFunctionBodies = 1;

  // Force color diagnostics if requested via environment variables
  // This ensures colors work even when output is redirected to a pipe
  if (std::getenv("CLICOLOR_FORCE") || std::getenv("LLVM_FORCE_COLOR")) { CI.getDiagnosticOpts().ShowColors = true; }

  return true;
}

// ------------------------------

custom_action::ASTConsumerPointer custom_action::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) {
  wdata = std::make_unique<wdata_t>(&CI, config);

  auto outfilename = wdata->module_info.sourcefile_full_stem + ".wrap.cxx";
  if (not std::filesystem::exists(outfilename)) std::ofstream{outfilename};
  // empty file to ensure that the tool can parse the include "mymodule.cxx"
  // if it has been removed for any reason.

  return std::make_unique<ast_consumer>(wdata);
}

// ------------------------------

void custom_action::ExecuteAction() {
  clang::Preprocessor &PP = getCompilerInstance().getPreprocessor();
  PP.addPPCallbacks(std::make_unique<pp_include_callback>(&getCompilerInstance().getASTContext(), *wdata.get()));
  ASTFrontendAction::ExecuteAction();
}

// ------------------------------

void custom_action::EndSourceFileAction() {

  util::logger log = util::logger{&std::cout, "-- ", ""};

  auto &ci = this->getCompilerInstance();
  if (ci.getASTContext().getDiagnostics().hasErrorOccurred()) return;

  auto code     = codegen_module(wdata->module_info);
  auto code_hxx = codegen_hxx(wdata->module_info);

  auto outfilename     = wdata->module_info.sourcefile_full_stem + ".wrap.cxx";
  auto outfilename_hxx = wdata->module_info.sourcefile_full_stem + ".wrap.hxx";

  log(fmt::format("\033[1;34m[Success] \033[0m"));
  log(fmt::format("   Generated Python bindings: {} [included in {}]", outfilename, wdata->module_info.sourcefile));
  log(fmt::format("             headers        : {} [to be used with other modules, cf doc]", outfilename_hxx));

  if (not std::getenv("CLAIR_SKIP_CLANG_FORMAT")) {
    log("Running clang-format ...");
    auto clang_format_style = clang::format::getStyle("file",                        // StyleName: look for .clang-format file
                                                      wdata->module_info.sourcefile, // FileName: directory to start search from
                                                      "LLVM"                         // Fallback style if no config found
                                                      )
                                 .get(); // because of FallBack the get is always valid

    code     = clu::clang_format(code, clang_format_style);
    code_hxx = clu::clang_format(code_hxx, clang_format_style);
    log("Done.");
  }

  std::ofstream(outfilename) << code;
  std::ofstream(outfilename_hxx) << code_hxx;

  // Examine if the preprocessor has found the include of the generated file in the module.
  if (not wdata->input_has_included_generated_cxx) {
    auto include_directive = fmt::format("\n#include \"{}\"\n", wdata->module_info.module_name + ".wrap.cxx");
    auto rewriter          = std::make_unique<clang::Rewriter>(wdata->ci->getSourceManager(), wdata->ci->getLangOpts());
    rewriter->InsertTextBefore(wdata->ci->getSourceManager().getLocForEndOfFile(wdata->ci->getSourceManager().getMainFileID()),
                               include_directive);
    rewriter->overwriteChangedFiles();
    log(fmt::format(
       "\033[1;95mModified the source\033[0m to add the missing include directive for the bindings at the end of \"\033[3;30m{}\033[0m\": \033[3;30m{}\033[0m",
       wdata->module_info.sourcefile, include_directive));
  }

  // If a depfile is specified, write the dependencies
  if (not config._depfile_name.empty()) {
    std::ofstream depfile(config._depfile_name);
    depfile << outfilename << ":";
    static const char *BR = "\\\n  ";
    depfile << BR << wdata->module_info.sourcefile;
    if (auto toml = wdata->module_info.sourcefile_full_stem + ".toml"; std::filesystem::exists(toml)) depfile << BR << toml;
    for (const auto &dep : wdata->deps) { depfile << BR << dep; }
    depfile << "\n";
    log(fmt::format("   Generated the depfile {}\n", config._depfile_name));
  }
}
