#pragma once
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "worker.hpp"
namespace matchers {

  using MatchCallback = clang::ast_matchers::MatchFinder::MatchCallback;
  using MatchResult   = clang::ast_matchers::MatchFinder::MatchResult;

  // A simple way to have several matchers. Just specialize the run method.
  // For the moment, only one matcher here.

  enum matcher_name_e { Fnt };

  template <matcher_name_e M> class matcher : public MatchCallback {
    worker_t *worker;

    public:
    matcher(worker_t *worker) : worker{worker} {}
    void run(const MatchResult &Result) override;
  };

} // namespace matchers
