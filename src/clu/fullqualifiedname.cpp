#include "fullqualifiedname.hpp"
#include "misc.hpp"

#include <clang/AST/QualTypeNames.h>
namespace clu {

  str_t gfqn_impl(clang::QualType const &t, clang::ASTContext &ctx) {
    clang::PrintingPolicy policy(ctx.getLangOpts());
    policy.SuppressUnwrittenScope = false;
    policy.SuppressScope          = false;
    //policy.FullyQualifiedName     = true; // Optional: if you want full namespace paths like ::std::array
    policy.PrintCanonicalTypes = true; // Desugar aliases

    return clang::TypeName::getFullyQualifiedName(t.getCanonicalType(), ctx, policy);
  }

  // ----------------------------------

  str_t get_fully_qualified_name(clang::QualType const &t, clang::ASTContext &ctx) {

    // clean the std::__1 and similar compiler dependent garbage in the std library ...
    auto clean_libc_mess = [](str_t s) {
      static const std::regex reg1{"std::__1::"};
      static const std::regex reg2{"std::__cxx11::"};
      s = std::regex_replace(s, reg1, "std::");
      s = std::regex_replace(s, reg2, "std::");
      return s;
    };

    if (t->isReferenceType())
      return clean_libc_mess(gfqn_impl(t->getPointeeType(), ctx)) + '&';
    else
      return clean_libc_mess(gfqn_impl(t, ctx));
  }

  // ----------------------------------------------

  str_t get_fully_qualified_name(clang::TypeDecl const *t) {
    return get_fully_qualified_name(clang::QualType{t->getTypeForDecl(), 0}, t->getASTContext());
  }

} // namespace clu
