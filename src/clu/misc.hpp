#pragma once

//#include "utility/macros.hpp"
#include "utility/string_tools.hpp"

#include "clang/AST/ASTContext.h"
//#include "clang/AST/Type.h"
//#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Attr.h"
#include "clang/Lex/Lexer.h"

namespace clu {

  // ---------- get_name_of_TemplateArgument ---------------------------
  // Extract the name of a TemplateArgument. It is a bit tricky, so worth a function
  //
  str_t get_name_of_TemplateArgument(clang::TemplateArgument const &a, clang::ASTContext const *ctx);
  // Given a source range, grab it as a string
  inline str_t get_source_range_as_string(clang::SourceRange const &sr, clang::ASTContext const *ctx) {
    return clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(sr), ctx->getSourceManager(), ctx->getLangOpts()).str();
  }

  // ------------- Diagnostics  -----------------

  // Emit error at the location loc
  // N : literal string. Error message
  // e.g. emit_error(d, "blablabla");
  template <unsigned N> void emit_error(clang::SourceLocation const &loc, clang::ASTContext const &ctx,
                                        const char (&FormatString)[N]) { //NOLINT
    clang::DiagnosticsEngine &DE = ctx.getDiagnostics();
    const auto ID                = DE.getCustomDiagID(clang::DiagnosticsEngine::Error, FormatString);
    clang::DiagnosticBuilder DB  = DE.Report(loc, ID);
  }

  // Emit error at the decl d.
  // N : literal string. Error message
  // e.g. emit_error(d, "blablabla");
  template <unsigned N> void emit_error(clang::Decl const *d, const char (&FormatString)[N]) { //NOLINT
    emit_error(d->getBeginLoc(), d->getASTContext(), FormatString);
  }

  // Same as emit_error, but with warning.
  template <unsigned N> void emit_warning(clang::Decl const *d, const char (&FormatString)[N]) { //NOLINT
    auto &ctx                    = d->getASTContext();
    clang::DiagnosticsEngine &DE = ctx.getDiagnostics();
    const auto ID                = DE.getCustomDiagID(clang::DiagnosticsEngine::Warning, FormatString);
    clang::DiagnosticBuilder DB  = DE.Report(d->getBeginLoc(), ID);
  }

  // ------------- Deprecation attribute --------------

  inline bool is_deprecated(const clang::Decl *d) {
    auto const *attr = d->getAttr<clang::DeprecatedAttr>();
    return attr; // if we want the message ... attr->getMessage().str();
  }

  // -------------has_annotation -----------------

  bool has_annotation(const clang::Decl *d, const char *annotation);

} // namespace clu
