#include "./matchers.hpp"
#include <clang/AST/DeclCXX.h>
#include "clang/Lex/Lexer.h"
#include <iostream>

namespace matchers {

  template <> void matcher<Call>::run(const MatchResult &Result) {

    auto *c = Result.Nodes.getNodeAs<clang::CallExpr>("call");
    auto *m = Result.Nodes.getNodeAs<clang::CXXMethodDecl>("m");
    // Only deprecated ?
    auto const *attr = m->getAttr<clang::DeprecatedAttr>();
    if (!c or !m or !attr) return;

    auto &ctx = m->getASTContext();
    auto &SM  = ctx.getSourceManager();
    // Find the first opening parenthesis in the call expression.
    auto OpenParenLoc = clang::Lexer::findLocationAfterToken(c->getBeginLoc(), clang::tok::l_paren, SM, ctx.getLangOpts(), false);
    OpenParenLoc      = OpenParenLoc.getLocWithOffset(-1); // We need the (,  not the token AFTER it, so we go 1 step backward
    // Find the closing parent of the call expr
    auto CloseParenLoc = c->getRParenLoc();

    // c->getSourceRange().dump(SM);
    // OpenParenLoc.dump(SM);
    // CloseParenLoc.dump(SM);
    worker->rewriter->ReplaceText(OpenParenLoc, 1, "[");
    worker->rewriter->ReplaceText(CloseParenLoc, 1, "]");
  }
} // namespace matchers
