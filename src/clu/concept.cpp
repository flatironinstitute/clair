#include "./concept.hpp"
#include "utility/macros.hpp"
#include <clang/AST/QualTypeNames.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/Template.h>

bool clu::satisfy_concept(clang::QualType const &ty, clang::ConceptDecl const *cpt, clang::CompilerInstance *ci) {
  EXPECTS(cpt);

  //ty.dump();

  // auto return type, peel it
  if (auto *ty2 = ty->getContainedDeducedType()) { return satisfy_concept(ty2->getDeducedType(), cpt, ci); }

  // auto parameter type
  if (auto *ty2 = llvm::dyn_cast_or_null<clang::SubstTemplateTypeParmType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

  // typedef
  if (auto *ty2 = llvm::dyn_cast_or_null<clang::TypedefType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

  // ElaboratedType
  if (auto *ty2 = llvm::dyn_cast_or_null<clang::ElaboratedType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

  // TemplateSpecializationType
  if (auto *ty2 = llvm::dyn_cast_or_null<clang::TemplateSpecializationType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

  /* // LValueReferenceType */
  //if (auto *ty2 = llvm::dyn_cast_or_null<clang::LValueReferenceType>(ty.getTypePtr())) {
  //return satisfy_concept(ty2->getPointeeType (), cpt, ci);
  //}

  llvm::SmallVector<const clang::Expr *, 1> constraint_exprs{cpt->getConstraintExpr()};
  llvm::SmallVector<clang::TemplateArgument, 1> targs{{ty}};
  clang::ConstraintSatisfaction s;

  // API change for clang >= 16 for calling the CheckConstraintSatisfaction method
#if LLVM_VERSION_MAJOR > 15
  clang::MultiLevelTemplateArgumentList targs_list{const_cast<clang::ConceptDecl *>(cpt), targs, true}; //NOLINT
  bool error = ci->getSema().CheckConstraintSatisfaction(cpt, constraint_exprs, targs_list, cpt->getSourceRange(), s);
#else
  bool error = ci->getSema().CheckConstraintSatisfaction(cpt, constraint_exprs, targs, cpt->getSourceRange(), s);
#endif
  EXPECTS_WITH_MESSAGE(not error, "CheckConstraintSatisfaction : internal error");

  //if (not s.IsSatisfied) ci->getSema().DiagnoseUnsatisfiedConstraint(s);
  return s.IsSatisfied;
}
