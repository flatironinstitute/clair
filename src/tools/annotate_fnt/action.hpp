#pragma once

#include <utility>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

#include "./ast_consumer.hpp"
#include "./config.hpp"

class custom_action : public clang::ASTFrontendAction {
  config_t config;
  std::shared_ptr<worker_t> worker; // why shared ???  why not worker_t ??

  public:
  using ASTConsumerPointer = std::unique_ptr<clang::ASTConsumer>;

  custom_action(config_t config) : config{std::move(config)} {}

  ASTConsumerPointer CreateASTConsumer(clang::CompilerInstance &compiler, llvm::StringRef Filename) override {
    worker = std::make_unique<worker_t>(&compiler, config);
    return std::make_unique<ast_consumer>(worker);
  }
  // Executed at the end after the traversal of the action.
  void EndSourceFileAction() override { worker->complete_rewrite(); }
};

// ----------------------------------------------

struct custom_action_factory : public clang::tooling::FrontendActionFactory {
  config_t config;
  custom_action_factory(config_t config) : config{std::move(config)} {}
  std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<custom_action>(config); }
};
