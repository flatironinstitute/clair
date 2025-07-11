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
#include "clu/clang_formatter.hpp"

#include "./clang_format_config.hpp"

// class custom_action : public clang::ASTFrontendAction {
//     config_t config;
//     std::shared_ptr<worker_t> worker; // why shared ???  why not worker_t ??

//     public:
//     using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

//     custom_action(config_t config) : config{std::move(config)} {}

//     ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) override {
//       worker = std::make_unique<worker_t>(&compiler, config);
//       return std::make_unique<ast_consumer>(worker);
//     }
//   };

class custom_action : public clang::ASTFrontendAction {
  std::shared_ptr<worker_t> worker;

  public:
  using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

  custom_action() = default;

  // --------------------------

  //  virtual bool PrepareToExecuteAction(clang::CompilerInstance & Compiler) override{}

  // skip function bodies. Gain in compiling time is small
  // virtual bool BeginInvocation(clang::CompilerInstance &CI) override {
  //   CI.getInvocation().getFrontendOpts().SkipFunctionBodies = 1;
  //   return true;
  // }

  // --------------------------

  ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef) override {
    worker = std::make_unique<worker_t>(&compiler);
    return std::make_unique<ast_consumer>(worker);
  }

  // --------------------------

  //bool ParseArgs(clang::CompilerInstance const &, const std::vector<std::string> &) override { return true; }

  // --------------------------

  void EndSourceFileAction() override {
    std::cerr << "Current working dir: " << std::filesystem::current_path() << "\n";

    auto &ci = this->getCompilerInstance();
    if (ci.getASTContext().getDiagnostics().hasErrorOccurred()) return;

    auto code     = codegen_module(worker->module_info);
    auto code_hxx = codegen_hxx(worker->module_info);

    auto outfilename     = worker->module_info.sourcefile_full_stem + ".wrap.cxx";
    auto outfilename_hxx = worker->module_info.sourcefile_full_stem + ".wrap.hxx";

    code     = clu::clang_format(code);
    code_hxx = clu::clang_format(code_hxx);
    std::ofstream(outfilename) << code;
    std::ofstream(outfilename_hxx) << code_hxx;
    std::cerr << "Generated Python bindings " << outfilename << std::endl;
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
