#include "./ast_consumer.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "./matchers.hpp"
#include "clu/misc.hpp"
#include "fmt/core.h"
#include "utility/logger.hpp"
#include <filesystem>

using clang::ast_matchers::MatchFinder;
static const struct {
  util::logger note = util::logger{&std::cout, "-- ", "\033[1;32mNote:  \033[0m"};
} logs;

void ast_consumer::HandleTranslationUnit(clang::ASTContext &ctx) {

  // Parsing just occurred. If error, we stop immediately
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  using namespace clang::ast_matchers; // or the AST Matcher expressions are cumbersome

  // ------- Match some concepts in c2py::concepts
  {
    matcher<mtch::Concept> ma{worker};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(hasName("c2py"), forEach(namespaceDecl(hasName("concepts"), forEach(namedDecl().bind("conceptDecl"))))), &ma);
    // a few concepts that may be present in know library
    mf.addMatcher(namedDecl(namedDecl().bind("conceptDecl"), hasName("h5::Storable")), &ma);
    mf.matchAST(ctx);
  }

  if (worker->force_instantiation_add_methods == nullptr) {
    // one the main technical concept in module.hpp is missing...
    // the user has probably forgot to include the c2py file
    auto &sm = worker->ci->getSourceManager();
    clu::emit_error(sm.getLocForEndOfFile(sm.getMainFileID()), ctx, "I did not find the c2py concepts. Internal error. Aborting.");
    return;
  }

  if (worker->HasHdf5) logs.note("Found Flatiron/h5 Storable concept. Will generate h5 code for all wrapped classes satisfying this concept.");

  // ------- Match automatically detected classes.
  {
    matcher<mtch::Cls> ma{worker};
    MatchFinder mf;
    if (auto const &s = worker->match_names; not s.empty())
      mf.addMatcher(cxxRecordDecl(matchesName(s), unless(isExpansionInSystemHeader())).bind("class"), &ma);
    //mf.addMatcher(namespaceDecl(unless(isExpansionInSystemHeader()), hasName(ns), forEach(cxxRecordDecl().bind("class"))), &ma);
    else
      mf.addMatcher(cxxRecordDecl(isExpansionInMainFile()).bind("class"), &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match automatically detected functions
  // except friend function, or method (will be done by inspecting the relevant classes)
  {
    matcher<mtch::Fnt> ma{worker};
    MatchFinder mf;
    // Excludes:
    // 1- everything included with -isystem (e.g. other libs)
    // 2- methods
    // 3- friend declarations
    auto excludes = unless(anyOf(isExpansionInSystemHeader(), cxxMethodDecl(), hasAncestor(friendDecl())));
    if (auto const &s = worker->match_names; not s.empty())
      mf.addMatcher(functionDecl(matchesName(s), excludes).bind("func"), &ma);
    else
      mf.addMatcher(functionDecl(isExpansionInMainFile(), excludes).bind("func"), &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the enums
  {
    matcher<mtch::Enum> ma{worker};
    MatchFinder mf;
    if (auto const &s = worker->match_names; not s.empty())
      mf.addMatcher(enumDecl(matchesName(s), unless(isExpansionInSystemHeader())).bind("en"), &ma);
    else
      mf.addMatcher(enumDecl(isExpansionInMainFile(), unless(isExpansionInSystemHeader())).bind("en"), &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the classes declared in module by using
  {
    matcher<mtch::ModuleClsWrap> ma{worker};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(isExpansionInMainFile(), hasName("c2py_module"), //
                                forEach(typeAliasDecl().bind("decl"))),
                  &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // -------- done ----------
  // NB must be run HERE, as it may require the DiagnosticsEngine...
  worker->run();
}
