#pragma once
#include "llvm/ADT/APFloat.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "wdata.hpp"

using MatchCallback = clang::ast_matchers::MatchFinder::MatchCallback;
using MatchResult   = clang::ast_matchers::MatchFinder::MatchResult;

enum class mtch { Concept, ModuleClsWrap, Cls, Fnt, Enum };

// cpp file implements for all M
template <auto M> class matcher : public MatchCallback {
  wdata_t *wdata;

  public:
  matcher(wdata_t *wdata) : wdata{wdata} {}
  void run(const MatchResult &Result) override;
};
