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
  name = arg.name().ToString();
  if (auto const *t = arg.GetType())
    type.name = codegen::fortran_type_to_cpp(t->AsFortran(), module_name);
}

// ---------------------------------------------------------------------------
// FunctionDecl — Helper 
// ---------------------------------------------------------------------------

static std::string get_linkage_name(sema::Symbol const &actual_sym,
                                    std::string const &module_name,
                                    sema::SubprogramDetails const &subprogram_details) {
  // Determine the external symbol name using the actual (not binding) procedure name —
  // mirrors the module-level procedure constructor.
  //   • BIND(C, NAME='foo') → 'foo' exactly
  //   • BIND(C)             → lowercase actual procedure name
  //   • no BIND(C)          → Flang-mangled "_QM{mod}P{actual_proc}"
  if (actual_sym.attrs().test(sema::Attr::BIND_C)) {
    auto const *bname = subprogram_details.bindName();
    if (bname && !bname->empty()) {
      return *bname;
    } else {\
      std::string lower = actual_sym.name().ToString();
      std::transform(lower.begin(), lower.end(), lower.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return lower;
    }
  }

  return codegen::flang_procedure_name(module_name, actual_sym.name().ToString());
}

// ---------------------------------------------------------------------------
// FunctionDecl — module-level procedure (subroutine or function)
// ---------------------------------------------------------------------------

FunctionDecl::FunctionDecl(sema::Symbol const &sym, std::string const &module_name) {
  auto const &sub = sym.get<sema::SubprogramDetails>();

  simple_name    = sym.name().ToString();
  qualified_name = module_name + "::" + simple_name;
  linkage_name   = get_linkage_name(sym, module_name, sub);

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
  linkage_name     = get_linkage_name(actual_sym, module_name, sub);

  if (sub.isFunction()) {
    auto const *t    = sub.result().GetType();
    return_type.name = t ? codegen::fortran_type_to_cpp(t->AsFortran(), module_name) : "void";
  } else {
    return_type.name = "void";
  }

  bool skip_pass = !is_nopass_; // first dummy arg is the passed-object (self)
  for (sema::Symbol *arg : sub.dummyArgs()) {
    if (!arg) continue;
    if (skip_pass) {
      skip_pass = false;
      // CLASS(T) / CLASS(*) dummy arguments are compiled by Flang with a descriptor/box ABI.
      // TYPE(T) dummy arguments are passed as a plain pointer.
      if (auto const *t = arg->GetType())
        self_is_polymorphic = t->IsPolymorphic();
      continue;
    }
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
