#pragma once

#include "utility/macros.hpp"
#include "utility/string_tools.hpp"

#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"

namespace clu {

  // Full qualification of a (class) type, including the template parameters
  // Also clean the std::__1 etc...

  str_t get_fully_qualified_name(clang::QualType const &t, clang::ASTContext &ctx);
  str_t get_fully_qualified_name(clang::TypeDecl const *t);

} // namespace clu
