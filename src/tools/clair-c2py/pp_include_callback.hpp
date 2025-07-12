#pragma once
#include <utility>

#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/SourceManager.h"
#include "./worker.hpp"
#include "./clu/misc.hpp"

using namespace clang;

class pp_include_callback : public PPCallbacks {
  SourceManager &SM;
  ASTContext *Ctx;

  int count = 0;
  worker_t &worker; // NOLINT

  public:
  explicit pp_include_callback(SourceManager &SM, ASTContext *Ctx, worker_t &worker) : SM(SM), Ctx(Ctx), worker{worker} {}

  void InclusionDirective(SourceLocation HashLoc, const Token &, StringRef FileName, bool, CharSourceRange, OptionalFileEntryRef, StringRef,
                          StringRef, const Module *, bool, SrcMgr::CharacteristicKind) override { // NOLINT
    if (SM.isInMainFile(HashLoc)) {
      worker.includes_generated_cxx = (FileName == worker.module_info.module_name + ".wrap.cxx");
      if (FileName == "c2py/c2py.hpp") {
        if (count == 0)
          worker.includes_c2py_first = true;
        else
          clu::emit_error(HashLoc, *Ctx, "c2py/c2py.hpp should be included before any other include in the file");
      }
    }
  }
};
