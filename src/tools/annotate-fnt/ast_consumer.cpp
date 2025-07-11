#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "./ast_consumer.hpp"
#include "./matchers.hpp"

using clang::ast_matchers::MatchFinder;

void ast_consumer::HandleTranslationUnit(clang::ASTContext &ctx) {

  // Parsing just occurred. If error, we stop immediately.
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  matchers::matcher<matchers::Fnt> ma{worker.get()};
  MatchFinder mf;
  using namespace clang::ast_matchers; // or the AST Matcher expressions are cumbersome

  // All functions which have a ancestor namespace config.ns,
  // excluding those defined in a compoundStmt, i.e. lambda ...
  // Should we ??
  mf.addMatcher(functionDecl(unless(hasAncestor(compoundStmt())), hasAncestor(namespaceDecl(hasName(worker->config.ns)))).bind("func"), &ma);
  //mf.addMatcher(namespaceDecl(unless(isExpansionInSystemHeader()), hasName(worker->config.ns),
  //                            forEachDescendant(functionDecl(unless(hasAncestor(compoundStmt()))))).bind("func"))),
  // NB the isExpansionInSystemHeader removes everything included with -isystem
  mf.matchAST(ctx);

  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // Add here anything that still requires the DiagnosticsEngine.
}
