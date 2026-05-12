#include "inject_bool_vartempl_specialization.hpp"

// ------------------------------

clang::VarTemplateSpecializationDecl *clu::inject_bool_vartempl_specialization(clang::Sema &S, clang::VarTemplateDecl *VTD,
                                                                  clang::QualType T, bool Value) {
  clang::ASTContext &Ctx = S.getASTContext();
  T                      = Ctx.getCanonicalType(T);              // findSpecialization keys on canonical types; canonicalize to avoid duplicates
  clang::TemplateArgument Arg(T);                                // wrap T as a TemplateArgument, the unit of template argument lists in Clang's AST
  void *InsertPos = nullptr;                                     // receives the hash-table insertion point if the lookup misses, reused by AddSpecialization
  if (auto *Spec = VTD->findSpecialization({Arg}, InsertPos)) return Spec; // already injected (or explicit in source): nothing to do
  auto *Spec = clang::VarTemplateSpecializationDecl::Create(Ctx, VTD->getDeclContext(), VTD->getBeginLoc(),
                                                            VTD->getLocation(), VTD, Ctx.BoolTy,
                                                            /*TInfo=*/nullptr, clang::SC_None, {Arg}); // allocate is_wrapped<T> decl, borrowing source locations from the template
  Spec->setConstexpr(true);                                      // mirrors the template's 'static constexpr bool'; required for concept evaluation
  Spec->setInit(new (Ctx) clang::CXXBoolLiteralExpr(Value, Ctx.BoolTy, clang::SourceLocation())); // NOLINT  set to true
  Spec->setTemplateSpecializationKind(clang::TSK_ExplicitSpecialization); // treat as user-written template<> constexpr bool is_wrapped<T> = true
  VTD->AddSpecialization(Spec, InsertPos);                       // register in the template's hash table at the position found above
  VTD->getDeclContext()->addDecl(Spec);                          // make visible in the c2py namespace for subsequent AST lookups
  return Spec;
}
