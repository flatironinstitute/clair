#pragma once
#include "llvm/ADT/APFloat.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "worker.hpp"

using MatchCallback = clang::ast_matchers::MatchFinder::MatchCallback;
using MatchResult   = clang::ast_matchers::MatchFinder::MatchResult;

enum class mtch { Concept, ModuleVars, ModuleFntDispatch, ModuleClsWrap, ModuleClsInfo, Cls, Fnt, Enum };

// cpp file implements for all M in mtch values...
template <auto M> class matcher : public MatchCallback {
  worker_t *worker;

  public:
  matcher(worker_t *worker) : worker{worker} {}
  void run(const MatchResult &Result) override;
};
