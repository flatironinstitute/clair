#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "./ast_consumer.hpp"
#include "./matchers.hpp"

using clang::ast_matchers::MatchFinder;

void ast_consumer::HandleTranslationUnit(clang::ASTContext &ctx) {

  // Parsing just occurred. If error, we stop immediately.
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  matchers::matcher<matchers::Call> ma{worker.get()};
  MatchFinder mf;
  using namespace clang::ast_matchers; // or the AST Matcher expressions are cumbersome

  // All call expression of an operator() of an class with name nda::.*
  mf.addMatcher(callExpr(unless(isExpansionInSystemHeader()),                              //
                         callee(cxxMethodDecl(ofClass(matchesName("nda::.*"))).bind("m")), //
                         hasArgument(0, declRefExpr().bind("caller")))
                   .bind("call"),
                &ma);
  // NB the isExpansionInSystemHeader removes everything included with -isystem
  mf.matchAST(ctx);

  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // Add here anything that still requires the DiagnosticsEngine.
}
