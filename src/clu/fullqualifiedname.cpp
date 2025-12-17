#include "fullqualifiedname.hpp"
#include "misc.hpp"
#include <regex>

#include <clang/AST/QualTypeNames.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>

namespace clu {

  str_t gfqn_impl1(clang::QualType t, clang::ASTContext &ctx) {

    clang::PrintingPolicy policy(ctx.getLangOpts()); // FIXME : built once only ? in workdata ?
    policy.SuppressUnwrittenScope = false;
    policy.SuppressScope          = false;
    policy.FullyQualifiedName     = true;
    //policy.PrintCanonicalTypes    = true;

    // if the type is an AutoType, get to the type
    if (auto *auto_type = t->getAs<clang::AutoType>()) { t = auto_type->getDeducedType(); }

    if (t.isNull()) return ""; // abort

    // if the type is a built in, just get its name, resolving all aliases.
    // cv-qualifiers live on the QualType wrapper, not on BuiltinType itself, so prepend them explicitly.
    if (auto *bu = t->getAs<clang::BuiltinType>()) { return (t.isConstQualified() ? "const " : "") + str_t{bu->getName(policy)}; }

    // If we have an alias or a using, just the name, do not introspect.
    if (t->getAs<clang::TypedefType>()) { return clang::TypeName::getFullyQualifiedName(t, ctx, policy); }

    // from now on, we only use the policy with canonical type, as a backup
#if LLVM_VERSION_MAJOR >= 21
    policy.PrintAsCanonical = true;
#else
    policy.PrintCanonicalTypes = true;
#endif

    // if the type is a template instantiation or specialization, we peel the template
    // and apply the function recursively to each types inside the <...>

    if (auto *spe = t->getAs<clang::TemplateSpecializationType>()) {

      // if this is a type alias template (like get_value_t<...>), desugar it to the underlying type
      if (auto *tmpl_decl = spe->getTemplateName().getAsTemplateDecl()) {
        if (llvm::isa<clang::TypeAliasTemplateDecl>(tmpl_decl)) { return get_fully_qualified_name(t.getCanonicalType(), ctx); }
      }

      auto treat_one_template_arg = [&ctx, &policy](clang::TemplateArgument const &targ) -> str_t {
        switch (targ.getKind()) {
          case clang::TemplateArgument::ArgKind::Type: return get_fully_qualified_name(targ.getAsType(), ctx);

          case clang::TemplateArgument::ArgKind::Expression: {
            auto *expr = targ.getAsExpr();
            clang::Expr::EvalResult result;
            if (expr->EvaluateAsInt(result, ctx)) {
              auto val = result.Val.getInt().getExtValue();
              if (expr->getType()->isCharType()) { return std::string("'") + static_cast<char>(val) + "'"; }
              return std::to_string(val);
            }
            break;
          }

          case clang::TemplateArgument::ArgKind::Integral: return std::to_string(targ.getAsIntegral().getExtValue());
          default: break;
        }
        // default : use policy
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
    return {};
  }

  // ----------------------------------

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

    auto result = gfqn_impl1(t, ctx);
    if (not result.empty()) return clean_libc_mess(result);

    // if our manual function succeed, use it, otherwise return to the clang default
    // the clang default will use canonical type everywhere which is always correct
    // but ugly
    clang::PrintingPolicy policy(ctx.getLangOpts());
    policy.SuppressUnwrittenScope = false;
    policy.SuppressScope          = false;
    policy.FullyQualifiedName     = true;
#if LLVM_VERSION_MAJOR >= 21
    policy.PrintAsCanonical = true;
#else
    policy.PrintCanonicalTypes = true;
#endif

    return clean_libc_mess(clang::TypeName::getFullyQualifiedName(t, ctx, policy));

    // Not very clear what the difference between the 2 functions is ...
    //return clean_libc_mess(clang::TypeName::getFullyQualifiedName(t, ctx, policy));
    //return clean_libc_mess(t.getAsString(policy));
  }

  // ----------------------------------------------

  str_t get_fully_qualified_name(clang::TypeDecl const *t, bool canonical) {
#if LLVM_VERSION_MAJOR >= 22
    if (auto *td = llvm::dyn_cast<clang::TagDecl>(t))
      return get_fully_qualified_name(t->getASTContext().getTagType(clang::ElaboratedTypeKeyword::None, std::nullopt, td, false), t->getASTContext(), canonical);
#endif
    return get_fully_qualified_name(clang::QualType{t->getTypeForDecl(), 0}, t->getASTContext(), canonical);
  }

} // namespace clu
