#include "./concept.hpp"
#include "utility/macros.hpp"
#include <clang/AST/QualTypeNames.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/Template.h>
#include <map>

bool clu::satisfy_concept(clang::QualType const &ty, clang::ConceptDecl const *cpt, clang::CompilerInstance *ci) {
  EXPECTS(cpt);

  if (ty.isNull() or ty->isUndeducedAutoType()) return false;
  //ty.dump();

  // Cache results: same (concept, canonical type) always yields the same answer.
  static std::map<std::pair<clang::ConceptDecl const *, void *>, bool> cache;
  auto const key = std::pair{cpt, ty.getCanonicalType().getAsOpaquePtr()};
  if (auto it = cache.find(key); it != cache.end()) return it->second;

  auto const compute = [&]() -> bool {
    // auto return type, peel it
    if (auto *ty2 = ty->getContainedDeducedType()) { return satisfy_concept(ty2->getDeducedType(), cpt, ci); }

    // auto parameter type
    if (auto *ty2 = llvm::dyn_cast_or_null<clang::SubstTemplateTypeParmType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

    // typedef
    if (auto *ty2 = llvm::dyn_cast_or_null<clang::TypedefType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

    // ElaboratedType (removed in LLVM 22, absorbed into TagType)
#if LLVM_VERSION_MAJOR < 22
    if (auto *ty2 = llvm::dyn_cast_or_null<clang::ElaboratedType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }
#endif

    // TemplateSpecializationType
    if (auto *ty2 = llvm::dyn_cast_or_null<clang::TemplateSpecializationType>(ty.getTypePtr())) { return satisfy_concept(ty2->desugar(), cpt, ci); }

    /* // LValueReferenceType */
    //if (auto *ty2 = llvm::dyn_cast_or_null<clang::LValueReferenceType>(ty.getTypePtr())) {
    //return satisfy_concept(ty2->getPointeeType (), cpt, ci);
    //}

#if LLVM_VERSION_MAJOR >= 21
    auto constraints = llvm::SmallVector{clang::AssociatedConstraint{cpt->getConstraintExpr()}};
#else
    auto constraints = llvm::SmallVector<const clang::Expr *>{cpt->getConstraintExpr()};
#endif
    llvm::SmallVector<clang::TemplateArgument, 1> targs{{ty}};
    clang::ConstraintSatisfaction s;

    // API change for clang >= 16 for calling the CheckConstraintSatisfaction method
    clang::MultiLevelTemplateArgumentList targs_list{const_cast<clang::ConceptDecl *>(cpt), targs, true}; //NOLINT
    bool error = ci->getSema().CheckConstraintSatisfaction(cpt, constraints, targs_list, cpt->getSourceRange(), s);
    //EXPECTS_WITH_MESSAGE(not error, "CheckConstraintSatisfaction : internal error");

    //if (not s.IsSatisfied) ci->getSema().DiagnoseUnsatisfiedConstraint(s);
    return !error and s.IsSatisfied;
  };

  return cache[key] = compute();
}

// ------------------------------

bool clu::concept_holder::is_satisfied_by(clang::QualType const &ty) const {
  return concept_decl and satisfy_concept(ty, concept_decl, ci);
}

bool clu::concept_holder::is_satisfied_by(clang::CXXRecordDecl const *cls) const {
  return concept_decl and satisfy_concept(cls, concept_decl, ci);
}
