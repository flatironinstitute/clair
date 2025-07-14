#pragma once

#include "utility/macros.hpp"
#include "utility/string_tools.hpp"

#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"

namespace clu {

  // these functions are non trivial, there are PrintPolicy details
  // set properly, spurious std::__1 and co to remove.

  /**
   * @brief Get the fully qualified name of a type
   * 
   * @param t Type
   * @param ctx 
   * @param canonical True for code generation, False for documentation purpose.
   * @return str_t 
   */
  str_t get_fully_qualified_name(clang::QualType const &t, clang::ASTContext &ctx, bool canonical = true);

  /**
   * @brief Get the fully qualified name of a type
   * 
   * @param t Type
   * @param canonical True for code generation, False for documentation purpose.
   * @return str_t 
   */
  str_t get_fully_qualified_name(clang::TypeDecl const *t, bool canonical = true);

} // namespace clu
