#pragma once
#include <utility>

#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/SourceManager.h"
#include "./worker.hpp"
#include "./clu/misc.hpp"

class pp_include_callback : public clang::PPCallbacks {
  clang::ASTContext *Ctx;
  int count = 0;
  worker_t &worker; // NOLINT

  public:
  explicit pp_include_callback(clang::ASTContext *Ctx, worker_t &worker) : Ctx(Ctx), worker{worker} {}

  void InclusionDirective(clang::SourceLocation loc, const clang::Token &, clang::StringRef FileName, bool, clang::CharSourceRange,
                          clang::OptionalFileEntryRef, clang::StringRef, clang::StringRef, const clang::Module *, bool,
                          clang::SrcMgr::CharacteristicKind) override { // NOLINT
    if (Ctx->getSourceManager().isInMainFile(loc)) {
      worker.includes_generated_cxx = (FileName == worker.module_info.module_name + ".wrap.cxx");
      if (count++ == 0 && FileName != "c2py/c2py.hpp") {
        clu::emit_error(loc, *Ctx, "c2py/c2py.hpp should be included before any other include in the file");
      }
    }
  }
};
