#include "fullqualifiedname.hpp"
#include "misc.hpp"

#include <clang/AST/QualTypeNames.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Attr.h>

namespace clu {

  //  -------------  Full qualification of a (class) type, including the template parameters
  // Given a type, maybe template or function prototype,
  // return its name with full qualification

  str_t gfqn_impl1(clang::QualType t, clang::ASTContext &ctx) {

    const clang::PrintingPolicy policy(ctx.getLangOpts()); // FIXME : built once only ? in workdata ?

    // if the type is an AutoType, get to the type
    if (auto *auto_type = t->getAs<clang::AutoType>()) { t = auto_type->getDeducedType(); }

    if (t.isNull()) return "UNRESOLVED AUTO";

    // if the type is a built in, just get its name, resolving all aliases.
    if (auto *bu = t->getAs<clang::BuiltinType>()) { return str_t{bu->getName(policy)}; }

    // If we have an alias or a using, just the name, do not introspect.
    if (auto *tdef = t->getAs<clang::TypedefType>()) { return clang::TypeName::getFullyQualifiedName(t, ctx, policy); }

    // if the type is a template instantiation or specialization, we peel the template
    // and apply the function recursively to each types inside the <...>
    //
    if (auto *spe = t->getAs<clang::TemplateSpecializationType>()) {

      auto treat_one_template_arg = [&ctx, &policy](clang::TemplateArgument const &targ) -> str_t {
        // integer
        if (targ.getKind() == clang::TemplateArgument::ArgKind::Integral) return std::to_string(targ.getAsIntegral().getExtValue());
        // expression
        if (targ.getKind() == clang::TemplateArgument::ArgKind::Expression) {
          return clu::get_source_range_as_string(targ.getAsExpr()->getExprLoc(), &ctx);
        }
        // a type
        if (targ.getKind() == clang::TemplateArgument::ArgKind::Type) return get_fully_qualified_name(targ.getAsType(), ctx);
        // else simply copy
        std::string res;
        llvm::raw_string_ostream out(res);
        targ.print(policy, out, true);
        return out.str();
      }; // end lambda

      return (t.isConstQualified() ? "const " : "")                                        // const or not
         + spe->getTemplateName().getAsTemplateDecl()->getQualifiedNameAsString()          // template name, qualified
         + '<' + util::join(spe->template_arguments(), treat_one_template_arg, ',') + '>'; // all arguments
    }

    // if the type is a function prototype, we also peel it recursively
    if (auto *funproto = t->getAs<clang::FunctionProtoType>()) {
      auto self = [&ctx](clang::QualType const &t2) { return get_fully_qualified_name(t2, ctx); };
      return self(funproto->getReturnType()) + '(' + util::join(funproto->getParamTypes(), self, ',') + ')';
    }

    // all other cases, we trust the clang provided function
    return clang::TypeName::getFullyQualifiedName(t, ctx, policy);
  }

  // ---------------------------------

  str_t gfqn_impl(clang::QualType const &t, clang::ASTContext &ctx) {
    const clang::PrintingPolicy policy(ctx.getLangOpts());

    // FIXME unfortunately clang::TypeName::getFullyQualifiedName
    // has a bug, cf issue9 test ...
    // we have to write a complicated function...

    //auto r1 = clang::TypeName::getFullyQualifiedName(t, ctx, policy);
    auto r2 = gfqn_impl1(t, ctx);

    //std::cout << " v1 : " << r1 << std::endl;
    //std::cout << " v2 : " << r2 << std::endl;
    //  return r;
    return r2;
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
