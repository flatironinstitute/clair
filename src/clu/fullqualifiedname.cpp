#include "fullqualifiedname.hpp"
#include "misc.hpp"
#include <regex>

#include <clang/AST/QualTypeNames.h>
namespace clu {

  str_t get_fully_qualified_name(clang::QualType const &t, clang::ASTContext &ctx, bool canonical) {

    if (t->isReferenceType()) return get_fully_qualified_name(t->getPointeeType(), ctx, canonical) + '&';

    // clean the std::__1 and similar compiler dependent garbage in the std library ...
    auto clean_libc_mess = [](str_t s) {
      static const std::regex reg1{"std::__1::"};
      static const std::regex reg2{"std::__cxx11::"};
      s = std::regex_replace(s, reg1, "std::");
      s = std::regex_replace(s, reg2, "std::");
      return s;
    };

    clang::PrintingPolicy policy(ctx.getLangOpts());
    policy.SuppressUnwrittenScope = false;
    policy.SuppressScope          = false;
    policy.FullyQualifiedName     = true;
    policy.PrintCanonicalTypes    = canonical;

    // Not very clear what the difference between the 2 functions is ...
    return clean_libc_mess(clang::TypeName::getFullyQualifiedName(t, ctx, policy));
    //return clean_libc_mess(t.getAsString(policy));
  }

  // ----------------------------------------------

  str_t get_fully_qualified_name(clang::TypeDecl const *t, bool canonical) {
    return get_fully_qualified_name(clang::QualType{t->getTypeForDecl(), 0}, t->getASTContext(), canonical);
  }

} // namespace clu
