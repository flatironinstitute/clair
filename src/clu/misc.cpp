#include "./misc.hpp"
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Attr.h>

#include <algorithm>
#include <ranges>

namespace stdr = std::ranges;

namespace clu {

  str_t get_name_of_TemplateArgument(clang::TemplateArgument const &a, clang::ASTContext const *ctx) {
    // For pack arguments, expand elements directly to avoid the <> wrapping
    // that TemplateArgument::print adds around packs.
    if (a.getKind() == clang::TemplateArgument::Pack) {
      str_t result;
      for (auto const &elem : a.pack_elements()) {
        if (!result.empty()) result += ", ";
        result += get_name_of_TemplateArgument(elem, ctx);
      }
      return result;
    }
    const clang::PrintingPolicy policy(ctx->getLangOpts());
    str_t s;
    llvm::raw_string_ostream out(s);
    a.print(policy, out, false);
    return out.str();
  }

  bool has_annotation(const clang::Decl *d, const char *annotation) {
    if (not d->hasAttrs()) return false;
    auto const &attrs = d->getAttrs();
    return std::ranges::any_of(attrs, [annotation](auto &att) {
      if (auto *an = llvm::dyn_cast_or_null<clang::AnnotateAttr>(att))
        return an->getAnnotation() == annotation;
      else
        return false;
    });
  }

  std::optional<std::string> get_annotation_value(const clang::Decl *d, std::string head) {
    auto full_prefix = head + ":";
    for (auto *att : d->getAttrs()) {
      if (auto *an = llvm::dyn_cast_or_null<clang::AnnotateAttr>(att)) {
        std::string_view annot = an->getAnnotation();
        if (annot.starts_with(full_prefix)) return std::string{annot.substr(full_prefix.size())};
      }
    }
    return {};
  }

} // namespace clu
