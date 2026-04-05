#pragma once
#include <clang/AST/DeclCXX.h>

// ========== A few helper functions to introspect C++ class/field declarations ===========

// Returns the in-class initializer expression for a field, or nullptr if none.
// Handles template instantiations by falling back to the primary template.
clang::Expr const *get_field_initializer(clang::FieldDecl const *f);
