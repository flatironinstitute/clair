#pragma once
#include "clang/Frontend/CompilerInstance.h"

namespace clu {
  /**
   * @brief Checks whether a type satisfy a concept
   *
   * @param ty The type
   * @param C The concept
   * @param ci  A compiler instance (we use its Sema)
   * @return C<ty>
   */
  bool satisfy_concept(clang::QualType const &ty, clang::ConceptDecl const *C, clang::CompilerInstance *ci);
  /**
  * @brief Checks where a Class/Struct satisfy a concept
  *
  * @param cls
  * @param cpt The concept C
  * @param ci  A compiler instance (we use its Sema)
  * @return C<cls>
  */
  inline bool satisfy_concept(clang::CXXRecordDecl const *cls, clang::ConceptDecl const *cpt, clang::CompilerInstance *ci) {
    return satisfy_concept(clang::QualType{cls->getTypeForDecl(), 0}, cpt, ci);
  }
} // namespace clu