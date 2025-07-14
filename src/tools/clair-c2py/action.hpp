#pragma once

#include <utility>
#include <fstream>
#include <regex>
#include <fmt/core.h>
#include <fmt/format.h>

#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

#include "./ast_consumer.hpp"
#include "codegen/module.hpp"
#include "utility/macros.hpp"
#include "utility/stl_complement.hpp"
#include "clu/clang_format.hpp"
#include "pp_include_callback.hpp"

class custom_action : public clang::ASTFrontendAction {
  std::shared_ptr<worker_t> worker;

  public:
  using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

  custom_action() = default;

  // --------------------------

  //  virtual bool PrepareToExecuteAction(clang::CompilerInstance & Compiler) override{}

  // skip function bodies. Gain in compiling time is small
  bool BeginInvocation(clang::CompilerInstance &CI) override {
    CI.getInvocation().getFrontendOpts().SkipFunctionBodies = 1;
    return true;
  }

  // --------------------------

  ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) override {
    worker = std::make_unique<worker_t>(&compiler);
    return std::make_unique<ast_consumer>(worker);
  }

  // --------------------------

  void ExecuteAction() override {
    clang::Preprocessor &PP = getCompilerInstance().getPreprocessor();
    PP.addPPCallbacks(std::make_unique<pp_include_callback>(&getCompilerInstance().getASTContext(), *worker.get()));
    ASTFrontendAction::ExecuteAction();
  }

  // --------------------------

  void EndSourceFileAction() override {

    util::logger log = util::logger{&std::cout, "-- ", ""};

    auto &ci = this->getCompilerInstance();
    if (ci.getASTContext().getDiagnostics().hasErrorOccurred()) return;

    auto code     = codegen_module(worker->module_info);
    auto code_hxx = codegen_hxx(worker->module_info);

    auto outfilename     = worker->module_info.sourcefile_full_stem + ".wrap.cxx";
    auto outfilename_hxx = worker->module_info.sourcefile_full_stem + ".wrap.hxx";

    auto clang_format_style = clang::format::getStyle("file",                         // StyleName: look for .clang-format file
                                                      worker->module_info.sourcefile, // FileName: directory to start search from
                                                      "LLVM"                          // Fallback style if no config found
                                                      )
                                 .get(); // because of FallBack the get is always valid

    code     = clu::clang_format(code, clang_format_style);
    code_hxx = clu::clang_format(code_hxx, clang_format_style);
    std::ofstream(outfilename) << code;
    std::ofstream(outfilename_hxx) << code_hxx;
    log(fmt::format("Generated Python bindings in files: {} and {}", outfilename, outfilename_hxx));

    // Examine if the preprocessor has found the include of the generated file in the module.
    if (not worker->includes_generated_cxx) {
      auto include_directive = fmt::format("\n#include \"{}\"\n", worker->module_info.module_name + ".wrap.cxx");
      auto rewriter          = std::make_unique<clang::Rewriter>(worker->ci->getSourceManager(), worker->ci->getLangOpts());
      rewriter->InsertTextBefore(worker->ci->getSourceManager().getLocForEndOfFile(worker->ci->getSourceManager().getMainFileID()),
                                 include_directive);
      rewriter->overwriteChangedFiles();
      log(fmt::format("Adding the include directive for the generated file {}\n in the main module file\n", include_directive));
    }
  }
};

// ----------------------------------------------

struct custom_action_factory : public clang::tooling::FrontendActionFactory {
  //config_t config;
  //custom_action_factory(config_t config) : config{std::move(config)} {}
  custom_action_factory() = default;

  std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<custom_action>(); }
  //std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<custom_action>(config); }
};
