#include "./ast_consumer.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <llvm/Support/raw_ostream.h>
#include <filesystem>
#include <ostream>
#include "fmt/core.h"

#include "clu/misc.hpp"
#include "utility/logger.hpp"
#include "./matchers.hpp"
#include "./scan_classes.hpp"

namespace fs = std::filesystem;
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
    matcher<mtch::Concept> ma{wdata};
    MatchFinder mf;
    mf.addMatcher(namespaceDecl(hasName("c2py"), forEach(namespaceDecl(hasName("concepts"), forEach(namedDecl().bind("conceptDecl"))))), &ma);
    // a few concepts that may be present in known library
    mf.addMatcher(namedDecl(namedDecl().bind("conceptDecl"), hasName("h5::Storable")), &ma);
    mf.matchAST(ctx);
  }
  if (not wdata->concepts.IsConvertiblePy2C) {
    llvm::errs() << "Can not find the c2py concepts. Internal error. It should never happen. Aborting.";
    return;
  }

  if (wdata->concepts.HasHdf5)
    logs.note("Found Flatiron/h5 Storable concept. Will generate h5 code for all wrapped classes satisfying this concept.");

  // ------- Build the matcher to restrict the match to the namespaces
  auto make_ns_matcher = [&]() -> DeclarationMatcher {
    // 1- Build nested namespaceDecl(hasName(...), hasDeclContext(...))
    // e.g. namespace A::B would yield
    // namespaceDecl(hasName("A"), hasDeclContext(namespaceDecl(hasName("B"))
    std::vector<DeclarationMatcher> ns_matcher_list;
    for (const auto &parts : wdata->config._namespaces_list) {
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
  // It is more readable than multiple if, besides all attempts
  // to use e.g. ternary have failed as matcher type vary

  // add the arguments for the names if the option is set
  auto add_name = [&](auto l, auto... x) {
    if (auto &s = wdata->config.match_names; !s.empty())
      return l(std::move(x)..., matchesName(s));
    else
      return l(std::move(x)...);
  };
  // add the arguments for namespace if the option is set
  auto add_ns = [&](auto l, auto... x) {
    if (not wdata->config._namespaces_list.empty()) {
      return add_name(l, std::move(x)..., hasDeclContext(make_ns_matcher()));
    } else
      return add_name(l, std::move(x)...);
  };
  // add the arguments for the file if the option is set
  auto add_match_files = [&](auto l, auto... x) {
    if (auto &s = wdata->config.match_files; !s.empty()) {
      //llvm::errs() << "Source file: " << fs::path{wdata->module_info.sourcefile}.filename() << "\n";
      //llvm::errs() << "Match files: " << wdata->config.match_files << "\n";
      // optimization : if the match file is exactly the main file, we can use isExpansionInMainFile which is much faster
      if (fs::path{wdata->module_info.sourcefile}.filename() == s)
        return add_ns(l, std::move(x)..., isExpansionInMainFile());
      else
        return add_ns(l, std::move(x)..., isExpansionInFileMatching(s));
    } else {
      return add_ns(l, std::move(x)...);
    }
  };
  // add the arguments for the names if the option is set
  auto add_excludes = [&](auto l) {
    if (auto &s = wdata->config.exclude_system_headers)
      return add_match_files(l, unless(isExpansionInSystemHeader()));
    else
      return add_match_files(l);
  };
  // Final call of the chain, for classes, enums and functions
  // Functions are special: exclude methods. Friend declarations (both inline
  // definitions and out-of-class declarations) are matched; deduplication
  // against out-of-class definitions is handled in the Fnt matcher callback.
  auto call_cls = [&](auto... x) {
    return cxxRecordDecl(std::move(x)...); //excludes);
  };
  auto call_enum = [&](auto... x) {
    return enumDecl(std::move(x)...); //excludes);
  };
  auto call_fun = [&](auto... x) {
    return functionDecl(std::move(x)..., unless(cxxMethodDecl()));
  };

  // ------- Match the AST in two passes
  // Pass 1: Match classes and enums first, so all classes are collected
  // before we process functions (which may reference these classes)
  MatchFinder mf1, mf2;
  matcher<mtch::Cls> ma_cls{wdata};
  matcher<mtch::Enum> ma_enum{wdata};
  matcher<mtch::ModuleClsWrap> ma_using{wdata};

  mf1.addMatcher(add_excludes(call_cls).bind("class"), &ma_cls);
  mf1.addMatcher(add_excludes(call_enum).bind("en"), &ma_enum);
  // Match using declarations in c2py_module namespace.
  // No file restriction: c2py_module is specific enough, and the declarations
  // may live in an #include'd file
  mf1.addMatcher(namespaceDecl(hasName("c2py_module"), forEach(typeAliasDecl().bind("decl"))), &ma_using);
  mf1.matchAST(ctx);
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // Pass 2: Match functions after that all classes are known
  matcher<mtch::Fnt> ma_f{wdata};
  mf2.addMatcher(add_excludes(call_fun).bind("func"), &ma_f);
  mf2.matchAST(ctx);
  if (ctx.getDiagnostics().hasErrorOccurred()) return;

  // -------- done ----------
  // NB must be run HERE, as it may require the DiagnosticsEngine...
  for (auto &[_, v] : wdata->module_info.functions) v = make_unique_decls(v);
  scan_classes(*wdata);
}
