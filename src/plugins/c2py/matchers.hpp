#pragma once
#include "llvm/ADT/APFloat.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "worker.hpp"
namespace matchers {

  using MatchCallback = clang::ast_matchers::MatchFinder::MatchCallback;
  using MatchResult   = clang::ast_matchers::MatchFinder::MatchResult;

  // FIXME : choose MACRO or template below ...

  enum MatcherList { Concept, ModuleVars, ModuleFntDispatch, ModuleClsWrap, ModuleClsInfo, Cls, Fnt, Enum };

  template <auto M> class matcher : public MatchCallback {
    worker_t *worker;

    public:
    matcher(worker_t *worker) : worker{worker} {}
    void run(const MatchResult &Result) override;
  };

// NOLINTNEXTLINE
#define CLAIR_DECLARE_MATCHER(CLS)                                                                                                                   \
  class CLS : public MatchCallback {                                                                                                                 \
    worker_t *worker;                                                                                                                                \
                                                                                                                                                     \
    public:                                                                                                                                          \
    CLS(worker_t *worker) : worker{worker} {}                                                                                                        \
    void run(const MatchResult &Result) override;                                                                                                    \
  };

  CLAIR_DECLARE_MATCHER(concept_t);
  CLAIR_DECLARE_MATCHER(module_vars_t);
  CLAIR_DECLARE_MATCHER(module_fun_dispatch_t);
  CLAIR_DECLARE_MATCHER(module_cls_wrap_t);
  CLAIR_DECLARE_MATCHER(module_cls_info_t);
  CLAIR_DECLARE_MATCHER(cls_t);
  //CLAIR_DECLARE_MATCHER(fnt_t);
  CLAIR_DECLARE_MATCHER(enum_t);

#undef CLAIR_DECLARE_MATCHER

} // namespace matchers
