#pragma once
#include <clang/AST/DeclTemplate.h>
#include <clang/Sema/Sema.h>

namespace clu {
  /// Injects an explicit specialization of a bool variable template V<T>
  /// with a given value, e.g. forcing c2py::is_wrapped<MyType> = true.
  /// No-op if a specialization already exists.
  clang::VarTemplateSpecializationDecl *inject_bool_vartempl_specialization(clang::Sema &S, clang::VarTemplateDecl *VTD,
                                                              clang::QualType T, bool Value);
} // namespace clu
