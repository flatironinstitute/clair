#include "./ast_consumer.hpp"

//#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
//#include "clang/Frontend/FrontendPluginRegistry.h"
//#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
//#include "clang/Parse/ParseAST.h"

// DEBUG
//#include "clang/Lex/Preprocessor.h"

//#include "clu/misc.hpp"
#include "./matchers.hpp"
#include "clu/misc.hpp"
#include "fmt/core.h"
#include "utility/logger.hpp"

using clang::ast_matchers::MatchFinder;
static const struct {
  util::logger note = util::logger{&std::cout, "-- ", "\033[1;32mNote:  \033[0m"};
} logs;

void ast_consumer::HandleTranslationUnit(clang::ASTContext &ctx) {

  // Parsing just occurred. If error, we stop
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  using namespace clang::ast_matchers; // or the AST Matcher expressions are cumbersome
  //auto & preproc = worker->ci->getPreprocessor();
  //PRINT(preproc.isMacroDefined ("ZOZO"));

  // ------- Match some concepts in c2py::concepts
  {
    matchers::concept_t ma{worker};
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
    clu::emit_error(sm.getLocForEndOfFile(sm.getMainFileID()), ctx, "Did you forget to include c2py/c2py.hpp ?");
    return;
  }

  if (worker->HasHdf5) logs.note("Found Flatiron/h5 Storable concept. Will generate h5 code for all wrapped classes satisfying this concept.");

  // ------- Match the config variables in c2py_module in main file
  // e.g. auto ns = "N"; etc...
  {
    matchers::module_vars_t vars{worker};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(isExpansionInMainFile(), hasName("c2py_module"), //
                                forEach(varDecl().bind("decl"))),
                  &vars);

    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match automatically detected classes.
  {
    matchers::cls_t ma{worker};
    MatchFinder mf;
    if (auto const &s = worker->module_info.match_names; not s.empty())
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
    matchers::matcher<matchers::Fnt> ma{worker};
    // NB the std header clause will remove everything included with -isystem (e.g. other libs)
    MatchFinder mf;
    if (auto const &s = worker->module_info.match_names; not s.empty())
      mf.addMatcher(functionDecl(matchesName(s), unless(anyOf(isExpansionInSystemHeader(), cxxMethodDecl(), hasAncestor(friendDecl())))).bind("func"),
                    &ma);
    else
      mf.addMatcher(
         functionDecl(isExpansionInMainFile(), unless(anyOf(isExpansionInSystemHeader(), cxxMethodDecl(), hasAncestor(friendDecl())))).bind("func"),
         &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the enums
  {
    matchers::enum_t ma{worker};
    MatchFinder mf;
    if (auto const &s = worker->module_info.match_names; not s.empty())
      mf.addMatcher(enumDecl(matchesName(s), unless(isExpansionInSystemHeader())).bind("en"), &ma);
    else
      mf.addMatcher(enumDecl(isExpansionInMainFile(), unless(isExpansionInSystemHeader())).bind("en"), &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the c2py::dispatch declarations in c2py_module::functions
  {
    matchers::module_fun_dispatch_t ma{worker};
    MatchFinder mf;
    // match the dispatch directly under add, not those of the nested struct
    mf.addMatcher(namespaceDecl(isExpansionInMainFile(), hasName("c2py_module"), //
                                has(namespaceDecl(hasName("add"), forEach(varDecl().bind("decl"))))),
                  &ma);

    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the classes declared in module by using
  {
    matchers::module_cls_wrap_t ma{worker};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(isExpansionInMainFile(), hasName("c2py_module"), //
                                forEach(typeAliasDecl().bind("decl"))),
                  &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // ------- Match the classes in c2py_module::add: in main file
  {
    matchers::module_cls_info_t ma{worker};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(hasName("c2py_module"), forEach(classTemplateDecl().bind("decl"))), &ma);
    mf.matchAST(ctx);
  }
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // -------- done ----------
  // NB must be run HERE, as it may require the DiagnosticsEngine...
  worker->run();
}
