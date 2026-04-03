#pragma once
#include <memory>
#include <utility>

#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

#include "./configuration.hpp"
#include "./wdata.hpp"

// -----------------------------------------------
// custom_action: FrontendAction that drives the full binding-generation pipeline.
// CreateASTConsumer sets up the ASTConsumer, ExecuteAction attaches the preprocessor
// callback, and EndSourceFileAction writes the generated .wrap.cxx/.wrap.hxx files.
class custom_action : public clang::ASTFrontendAction {
  std::shared_ptr<wdata_t> wdata;
  configuration config;

  public:
  using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

  custom_action(configuration config) : config{std::move(config)} {}

  bool BeginInvocation(clang::CompilerInstance &CI) override;
  ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override;
  void ExecuteAction() override;
  void EndSourceFileAction() override;
};

// -----------------------------------------------
// Factory that creates one custom_action per translation unit.
struct custom_action_factory : public clang::tooling::FrontendActionFactory {
  configuration config;
  custom_action_factory(configuration config) : config{std::move(config)} {}
  std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<custom_action>(config); }
};
