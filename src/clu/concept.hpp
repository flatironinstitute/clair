#pragma once
#include "clang/AST/DeclCXX.h"
#include "clang/Frontend/CompilerInstance.h"

namespace clu {

  /// Owns a concept declaration and a compiler instance; provides an is_satisfied_by() method.
  /// If concept_decl is null (concept not found in the TU), is_satisfied_by() always returns false.
  struct concept_holder {
    clang::ConceptDecl const *concept_decl = nullptr;
    clang::CompilerInstance *ci            = nullptr;

    explicit operator bool() const noexcept { return concept_decl != nullptr; }
    bool is_satisfied_by(clang::QualType const &ty) const;
    bool is_satisfied_by(clang::CXXRecordDecl const *cls) const;
  };
  /**
   * @brief Checks whether a type satisfies a concept
   *
   * @param ty The type
   * @param C The concept
   * @param ci  A compiler instance (we use its Sema)
   * @return C<ty>
   */
  bool satisfy_concept(clang::QualType const &ty, clang::ConceptDecl const *C, clang::CompilerInstance *ci);
  /**
  * @brief Checks whether a Class/Struct satisfies a concept
  *
  * @param cls
  * @param cpt The concept C
  * @param ci  A compiler instance (we use its Sema)
  * @return C<cls>
  */
  inline bool satisfy_concept(clang::CXXRecordDecl const *cls, clang::ConceptDecl const *cpt, clang::CompilerInstance *ci) {
#if LLVM_VERSION_MAJOR >= 22
    return satisfy_concept(clang::QualType(cls->getASTContext().getCanonicalTagType(cls)), cpt, ci);
#else
    return satisfy_concept(clang::QualType{cls->getTypeForDecl(), 0}, cpt, ci);
#endif
  }
} // namespace clu