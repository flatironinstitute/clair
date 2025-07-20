#include "./ast_consumer.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"

#include "./matchers.hpp"
#include "clu/misc.hpp"
#include "fmt/core.h"
#include "utility/logger.hpp"
#include <filesystem>
#include <llvm/Support/raw_ostream.h>
#include <ostream>

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
    // a few concepts that may be present in known library
    mf.addMatcher(namedDecl(namedDecl().bind("conceptDecl"), hasName("h5::Storable")), &ma);
    mf.matchAST(ctx);
  }
  if (worker->IsConvertiblePy2C == nullptr) {
    llvm::errs() << "Can not find the c2py concepts. Internal error. It should never happen. Aborting.";
    return;
  }

  if (worker->HasHdf5) logs.note("Found Flatiron/h5 Storable concept. Will generate h5 code for all wrapped classes satisfying this concept.");

  // ------- Build the matcher to restrict the match to the namespaces
  auto make_ns_matcher = [&]() -> DeclarationMatcher {
    // 1- Build nested namespaceDecl(hasName(...), hasDeclContext(...))
    // e.g. namspace A::B would yield
    // namespaceDecl(hasName("A"), hasDeclContext(namespaceDecl(hasName("B"))
    std::vector<DeclarationMatcher> ns_matcher_list;
    for (const auto &parts : worker->config._namespaces_list) {
      DeclarationMatcher m = namespaceDecl(hasName(parts[0]));
      for (int i = 1; i < int(parts.size()); ++i) { m = namespaceDecl(hasName(parts[i]), hasDeclContext(m)); }
      ns_matcher_list.push_back(m);
    }
    // 2- Make a single matcher for all namespaces as AnyOf
    DeclarationMatcher ns_matcher = ns_matcher_list[0];                                                          // start with first
    for (std::size_t i = 1; i < ns_matcher_list.size(); ++i) ns_matcher = anyOf(ns_matcher, ns_matcher_list[i]); // add one matcher at a time
    return ns_matcher;
  };

  // ---- Build the matchers for classes, enums and functions
  // The matchers are quite complex, as we have options
  // to match the namespaces, the names, and the files, for classes, functions, enums.
  // There are many possible matchers, and their types are different.
  // I use a chain of lambda to build the correct call to FunctionDecl et al
  // in each case by progressively accumulating the arguments in a parameter pack.
  // This is not the most efficient way, but it is the most readable.
  // It is in fact more readable than multiple if, besides all attempts (including with AI)
  // to use e.g. ternary have failed as matcher type vary
  // nb the order of the chain is unimportant.

  // add the arguments for namespace if the option is set
  auto add_ns = [&](auto l, auto... x) {
    if (not worker->config._namespaces_list.empty()) {
      return l(hasDeclContext(make_ns_matcher()), std::move(x)...);
    } else
      return l(std::move(x)...);
  };
  // add the arguments for the names if the option is set
  auto add_name = [&](auto l, auto... x) {
    if (auto &s = worker->config.match_names; !s.empty())
      return add_ns(l, matchesName(s), std::move(x)...);
    else
      return add_ns(l, std::move(x)...);
  };
  // add the arguments for the file if the option is set
  auto add_match_files = [&](auto l, auto... x) {
    if (auto &s = worker->config.match_files; !s.empty())
      return add_name(l, isExpansionInFileMatching(s), std::move(x)...);
    else {
      // if no match_files and no reject_names, we restrict to the main file (for test mainly)
      if (worker->config.match_names.empty() and worker->config.reject_names.empty())
        return add_name(l, isExpansionInMainFile(), std::move(x)...);
      else
        return add_name(l, std::move(x)...);
    }
  };

  // Final call of the chain, for classes, enums and functions
  // Function are special, as we have to exclude methods and friend declarations
  auto call_cls = [&](auto... x) {
    auto excludes = unless(isExpansionInSystemHeader());
    return cxxRecordDecl(std::move(x)..., excludes);
  };

  auto call_enum = [&](auto... x) {
    auto excludes = unless(isExpansionInSystemHeader());
    return enumDecl(std::move(x)..., excludes);
  };

  auto call_fun = [&](auto... x) {
    auto excludes = unless(anyOf(isExpansionInSystemHeader(), cxxMethodDecl(), hasAncestor(friendDecl())));
    return functionDecl(std::move(x)..., excludes);
  };

  // ------- Match the AST
  MatchFinder mf;

  matcher<mtch::Cls> ma_cls{worker};
  matcher<mtch::Enum> ma_enum{worker};
  matcher<mtch::Fnt> ma_f{worker};
  matcher<mtch::ModuleClsWrap> ma_using{worker};

  mf.addMatcher(add_match_files(call_cls).bind("class"), &ma_cls);
  mf.addMatcher(add_match_files(call_enum).bind("en"), &ma_enum);
  mf.addMatcher(add_match_files(call_fun).bind("func"), &ma_f);

  mf.addMatcher(namespaceDecl(isExpansionInMainFile(), hasName("c2py_module"), //
                              forEach(typeAliasDecl().bind("decl"))),
                &ma_using);

  mf.matchAST(ctx);
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // -------- done ----------
  // NB must be run HERE, as it may require the DiagnosticsEngine...
  worker->run();
}
