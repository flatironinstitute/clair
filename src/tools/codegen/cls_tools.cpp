#include "cls_tools.hpp"

// ------------------------------

clang::Expr const *get_field_initializer(clang::FieldDecl const *f) {
  if (clang::Expr const *init = f->getInClassInitializer()) return init; // non-template structs

  // If f is from a template instantiation, find the original field in the primary template
  if (auto const *cls = dyn_cast<clang::CXXRecordDecl>(f->getParent()))
    if (auto const *tip = cls->getTemplateInstantiationPattern())
      for (clang::FieldDecl const *f_tpl : tip->fields())
        if (f_tpl->getName() == f->getName()) return f_tpl->getInClassInitializer();
  return nullptr;
}
