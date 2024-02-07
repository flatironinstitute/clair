#include "./matchers.hpp"
namespace matchers {

  template <> void matcher<Fnt>::run(const MatchResult &Result) {

    auto *f = Result.Nodes.getNodeAs<clang::FunctionDecl>("func");
    if (!f) return;

    // ............. Discard some automatic instantiation from the compiler, and alike

    // f in e.g. operator new, internal function, not defined in the sources
    if (!f->getBeginLoc().isValid()) return;

    // Skip automatic template instantiation by the compiler
    if (f->isFunctionTemplateSpecialization()) return;

    // Skip deleted function
    if (f->isDeleted()) return;

    // skip the deduction guides (CTAD)
    if (llvm::dyn_cast_or_null<clang::CXXDeductionGuideDecl>(f)) return;

    // ............. Now do something ...

    worker->rewriter->InsertTextBefore(f->getSourceRange().getBegin(), worker->config.annotation);
  }
} // namespace matchers
