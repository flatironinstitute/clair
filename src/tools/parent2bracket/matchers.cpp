#include "./matchers.hpp"
#include <clang/AST/DeclCXX.h>
#include "clang/Lex/Lexer.h"
#include <clang/AST/Stmt.h>
#include <iostream>

namespace matchers {

  template <> void matcher<Call>::run(const MatchResult &Result) {

    auto *c      = Result.Nodes.getNodeAs<clang::CallExpr>("call");
    auto *m      = Result.Nodes.getNodeAs<clang::CXXMethodDecl>("m");
    auto *caller = Result.Nodes.getNodeAs<clang::DeclRefExpr>("caller");
    // Only deprecated ?
    auto const *attr = m->getAttr<clang::DeprecatedAttr>();
    if (!c or !m or !attr) return;

    auto &ctx = m->getASTContext();
    auto &SM  = ctx.getSourceManager();

    // Find the first opening parenthesis in the call expression after the caller
    // (in case the caller itself has a () like (a)[1,2]. we can not start at the beginning of the expr itself.
    auto OpenParenLoc = clang::Lexer::findLocationAfterToken(caller->getBeginLoc(), clang::tok::l_paren, SM, ctx.getLangOpts(), false);
    OpenParenLoc      = OpenParenLoc.getLocWithOffset(-1);
    // We need the (,  not the token AFTER it, so we go 1 step backward
    // It seems that if ( is followed by space as in a( 1, 2) (not proper clang formatted anyway)
    // the space is part of the token after (, so shifting the location by 1 char is ok.

    // ok too, but the previous method is simpler.
    // clang::Token tok{};
    // auto OpenParenLoc = caller->getBeginLoc();
    // do { //NOLINT
    //   tok          = clang::Lexer::findNextToken(OpenParenLoc, SM, ctx.getLangOpts()).value();
    //   OpenParenLoc = tok.getLocation();
    // } while (not tok.is(clang::tok::l_paren));

    // Find the closing parent of the call expr
    auto CloseParenLoc = c->getRParenLoc();

    c->getSourceRange().dump(SM);
    // OpenParenLoc.dump(SM);
    // CloseParenLoc.dump(SM);
    worker->rewriter->ReplaceText(OpenParenLoc, 1, "[");
    worker->rewriter->ReplaceText(CloseParenLoc, 1, "]");
  }
} // namespace matchers
