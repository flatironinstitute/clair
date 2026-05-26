#include "types.hpp"

#include <algorithm>
#include "flang/Semantics/attr.h"
#include "flang/Semantics/symbol.h"
#include "flang/Semantics/type.h"
#include "tools/clair/codegen/utils.hpp"

namespace sema = Fortran::semantics;

namespace ir {

// ---------------------------------------------------------------------------
// ParamVarDecl
// ---------------------------------------------------------------------------

ParamVarDecl::ParamVarDecl(sema::Symbol const &arg, std::string const &module_name)
    : is_fortran_value(arg.attrs().test(sema::Attr::VALUE)) {
  name             = arg.name().ToString();
  if (auto const *t = arg.GetType())
    type.name = codegen::fortran_type_to_cpp(t->AsFortran(), module_name);
}

// ---------------------------------------------------------------------------
// FunctionDecl — module-level procedure (subroutine or function)
// ---------------------------------------------------------------------------

FunctionDecl::FunctionDecl(sema::Symbol const &sym, std::string const &module_name) {
  auto const &sub = sym.get<sema::SubprogramDetails>();

  simple_name    = sym.name().ToString();
  qualified_name = module_name + "::" + simple_name;

  // Determine the external symbol name:
  //  • BIND(C, NAME='foo') → 'foo' exactly
  //  • BIND(C)             → lowercase procedure name (Fortran standard §18.10.2)
  //  • no BIND(C)          → Flang-mangled "_QM{mod}P{proc}"
  if (sym.attrs().test(sema::Attr::BIND_C)) {
    auto const *bname = sub.bindName();
    if (bname && !bname->empty()) {
      linkage_name = *bname;
    } else {
      std::string lower = simple_name;
      std::transform(lower.begin(), lower.end(), lower.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      linkage_name = std::move(lower);
    }
  } else {
    linkage_name = codegen::flang_procedure_name(module_name, simple_name);
  }

  if (sub.isFunction()) {
    auto const *t    = sub.result().GetType();
    return_type.name = t ? codegen::fortran_type_to_cpp(t->AsFortran(), module_name) : "void";
  } else {
    return_type.name = "void";
  }

  for (sema::Symbol *arg : sub.dummyArgs()) {
    if (!arg) continue; // alternate-return indicator
    params.emplace_back(*arg, module_name);
  }
}

// ---------------------------------------------------------------------------
// FunctionDecl — type-bound procedure (method)
// ---------------------------------------------------------------------------

FunctionDecl::FunctionDecl(sema::Symbol const &binding_sym,
                            sema::Symbol const &actual_sym,
                            std::string const &module_name,
                            std::string const parent_fqn,
                            bool is_nopass_)
    : is_method(true), is_static(is_nopass_), parent_class_fqn(std::move(parent_fqn)) {
  auto const &sub = actual_sym.get<sema::SubprogramDetails>();

  simple_name      = binding_sym.name().ToString();
  qualified_name   = module_name + "::" + actual_sym.name().ToString();
  
  if (sub.isFunction()) {
    auto const *t    = sub.result().GetType();
    return_type.name = t ? codegen::fortran_type_to_cpp(t->AsFortran(), module_name) : "void";
  } else {
    return_type.name = "void";
  }

  bool skip_pass = !is_nopass_; // first dummy arg is the passed-object (self)
  for (sema::Symbol *arg : sub.dummyArgs()) {
    if (!arg) continue;
    if (skip_pass) { skip_pass = false; continue; }
    params.emplace_back(*arg, module_name);
  }
}

// ---------------------------------------------------------------------------
// RecordDecl
// ---------------------------------------------------------------------------

RecordDecl::RecordDecl(sema::Symbol const &sym, std::string const &module_name) : is_aggregate(true) {
  // is_aggregate = true: Fortran derived types have no user-provided constructors
  fully_qualified_name = module_name + "::" + sym.name().ToString();
}

// ---------------------------------------------------------------------------
// FieldDecl
// ---------------------------------------------------------------------------

FieldDecl::FieldDecl(sema::Symbol const &component, std::string const &module_name) {
  name = component.name().ToString();
  if (auto const *t = component.GetType())
    type.name = codegen::fortran_type_to_cpp(t->AsFortran(), module_name);
}

} // namespace ir
