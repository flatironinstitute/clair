#pragma once
#include <utility>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "config.hpp"

struct worker_t {
  config_t config;
  std::unique_ptr<clang::Rewriter> rewriter;

  worker_t(clang::CompilerInstance *ci, config_t config) : config{std::move(config)} {
    rewriter = std::make_unique<clang::Rewriter>(ci->getSourceManager(), ci->getLangOpts());
  }

  void complete_rewrite() { rewriter->overwriteChangedFiles(); }
};
