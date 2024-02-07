#include "./misc.hpp"
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Attr.h>

namespace clu {

  str_t get_name_of_TemplateArgument(clang::TemplateArgument const &a, clang::ASTContext const *ctx) {
    const clang::PrintingPolicy policy(ctx->getLangOpts());
    str_t s;
    llvm::raw_string_ostream out(s);
    a.print(policy, out, true);
    return out.str();
  }

  bool has_annotation(const clang::Decl *d, const char *annotation) {
    if (not d->hasAttrs()) return false;
    auto const &attrs = d->getAttrs();
    return std::any_of(attrs.begin(), attrs.end(), [annotation](auto &att) {
      if (auto an = llvm::dyn_cast_or_null<clang::AnnotateAttr>(att))
        return an->getAnnotation() == annotation;
      else
        return false;
    });
  }

} // namespace clu
