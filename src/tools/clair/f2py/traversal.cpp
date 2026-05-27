#include "traversal.hpp"
#include "decl_utils.hpp"
#include "flang/Semantics/attr.h"
#include "flang/Semantics/scope.h"
#include "flang/Semantics/symbol.h"
#include "flang/Semantics/type.h"

namespace sema = Fortran::semantics;

// ---------------------------------------------------------------------------

static void process_subprogram(sema::Symbol const &sym,
                                std::string const &module_name,
                                module_info_t &mi) {
  fnt_ptr_t ptr = mi.intern(ir::FunctionDecl{sym, module_name});
  mi.functions[ptr->simple_name].push_back(fnt_info_t{ptr});
}

// ---------------------------------------------------------------------------

static void process_derived_type(sema::Symbol const &sym,
                                  sema::Scope const &parentScope,
                                  std::string const &module_name,
                                  module_info_t &mi) {
  ir::RecordDecl rec{sym, module_name};

  mi.add_class(get_python_name_cls(sym), rec);
  cls_info_t *info = mi.get_wrapped_cls_info(rec.fully_qualified_name);
  if (!info) return;

  for (sema::Scope const &child : parentScope.children()) {
    if (!child.IsDerivedType() || child.symbol() != &sym) continue;

    // --- Data components ---
    for (sema::SourceName const &compName : sym.get<sema::DerivedTypeDetails>().componentNames()) {
      auto it = child.find(compName);
      if (it == child.end()) continue;
      info->fields.push_back(mi.intern(ir::FieldDecl{it->second.get(), module_name}));
    }

    // --- Type-bound procedures ---
    for (auto const &[bindingName, bindingRef] : child) {
      sema::Symbol const &bindingSym = bindingRef.get();
      if (!bindingSym.has<sema::ProcBindingDetails>()) continue;

      sema::Symbol const &actual = bindingSym.get<sema::ProcBindingDetails>().symbol();
      if (!actual.has<sema::SubprogramDetails>()) continue;

      bool is_nopass = bindingSym.attrs().test(sema::Attr::NOPASS);

      fnt_ptr_t ptr = mi.intern(
          ir::FunctionDecl{bindingSym, actual, module_name, rec.fully_qualified_name, is_nopass});
      info->methods[ptr->simple_name].push_back(
          fnt_info_t{ptr, /*rewrite=*/true, mi.get_wrapped_cls(rec.fully_qualified_name)});
    }

    break;
  }
}

// ---------------------------------------------------------------------------

void process_module_scope(sema::Scope const &modScope,
                           std::string const &module_name,
                           module_info_t &mi) {
  for (auto const &[name, symRef] : modScope) {
    sema::Symbol const &sym = symRef.get();

    // TODO: also accept symbols with no explicit access when the module's
    // default accessibility is PUBLIC (scope.IsDefaultPrivate() == false).
    if (!sym.attrs().test(sema::Attr::PUBLIC)) continue;

    if (sym.has<sema::SubprogramDetails>())
      process_subprogram(sym, module_name, mi);
    else if (sym.has<sema::DerivedTypeDetails>())
      process_derived_type(sym, modScope, module_name, mi);
    else if (sym.has<sema::ProcBindingDetails>()) {
      // Atypical: ProcBindingDetails at module scope (e.g. a USE re-export).
      sema::Symbol const &actual = sym.get<sema::ProcBindingDetails>().symbol();
      if (actual.has<sema::SubprogramDetails>())
        process_subprogram(actual, module_name, mi);
    }
  }
}
