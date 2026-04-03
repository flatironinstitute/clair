#pragma once
#include <utility>

#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/SourceManager.h"
#include "./wdata.hpp"
#include "./clu/misc.hpp"

class pp_include_callback : public clang::PPCallbacks {
  clang::ASTContext *Ctx;
  int count = 0;
  wdata_t &wdata; // NOLINT

  std::string escapeMakePath(llvm::StringRef path) {
    llvm::SmallString<128> escaped;
    for (char c : path) {
      switch (c) {
        case ' ': escaped.append("\\ "); break;
        case '\\': escaped.append("\\\\"); break;
        case '#': escaped.append("\\#"); break;
        case '$': escaped.append("$$"); break; // Makefile syntax: $$ produces a literal $ default: escaped.push_back(c); break;
        default: escaped.push_back(c); break;
      }
    }
    return {escaped};
  }

  public:
  explicit pp_include_callback(clang::ASTContext *Ctx, wdata_t &wdata) : Ctx(Ctx), wdata{wdata} {}

  void InclusionDirective(clang::SourceLocation loc, const clang::Token &, clang::StringRef FileName, bool, clang::CharSourceRange,
                          clang::OptionalFileEntryRef File, clang::StringRef, clang::StringRef, const clang::Module *, bool,
                          clang::SrcMgr::CharacteristicKind FileType) override { // NOLINT
    if (Ctx->getSourceManager().isInMainFile(loc)) {
      wdata.input_has_included_generated_cxx = (FileName == wdata.module_info.module_name + ".wrap.cxx");
      if (count++ == 0 && FileName != "c2py/c2py.hpp") {
        clu::emit_error(loc, *Ctx, "c2py/c2py.hpp should be included before any other include in the file");
      }
    }

    llvm::StringRef real    = File->getFileEntry().tryGetRealPathName();
    llvm::StringRef absPath = real.empty() ? File->getName() : real;

    if (FileType == clang::SrcMgr::C_User) { // only the user includes with -I. Ignore -isystem includes. It is a choice ...
      // Remove a circular dependency the wrap.cxx file.
      // It is the reason we can not use the CI.getDependencyOutputOpts, and we need to write the dependencies ourselves.
      // The tool reads the module.cpp, which includes the generated file, so the generated file would be a dependency of itself.
      if (FileName != wdata.module_info.module_name + ".wrap.cxx") wdata.deps.emplace_back(escapeMakePath(absPath));
    }
  }
};
