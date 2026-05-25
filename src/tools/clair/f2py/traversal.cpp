#include "traversal.hpp"

#include "flang/Semantics/attr.h"
#include "flang/Semantics/scope.h"
#include "flang/Semantics/symbol.h"
#include "flang/Semantics/type.h"

namespace sema = Fortran::semantics;

// ---------------------------------------------------------------------------

static void process_subprogram(sema::Symbol const &sym,
                                std::string const &module_name,
                                module_info_t &mi) {
  auto const &sub = sym.get<sema::SubprogramDetails>();

  ir::FunctionDecl fd;
  fd.simple_name    = sym.name().ToString();
  fd.qualified_name = module_name + "::" + fd.simple_name;

  if (sub.isFunction()) {
    auto const *t       = sub.result().GetType();
    fd.return_type.name = t ? t->AsFortran() : "void";
  } else {
    fd.return_type.name = "void";
  }

  for (sema::Symbol *arg : sub.dummyArgs()) {
    if (!arg) continue; // alternate-return indicator
    ir::ParamVarDecl p;
    p.name = arg->name().ToString();
    if (auto const *t = arg->GetType())
      p.type.name = t->AsFortran();
    fd.params.push_back(std::move(p));
  }

  fnt_ptr_t ptr = mi.intern(std::move(fd));
  mi.functions[ptr->simple_name].push_back(fnt_info_t{ptr});
}

// ---------------------------------------------------------------------------

static void process_derived_type(sema::Symbol const &sym,
                                  sema::Scope const &parentScope,
                                  std::string const &module_name,
                                  module_info_t &mi) {
  ir::RecordDecl rec;
  rec.fully_qualified_name = module_name + "::" + sym.name().ToString();
  rec.is_aggregate         = true;

  mi.add_class(rec.fully_qualified_name, rec);
  cls_info_t *info = mi.get_wrapped_cls_info(rec.fully_qualified_name);
  if (!info) return;

  for (sema::Scope const &child : parentScope.children()) {
    if (!child.IsDerivedType() || child.symbol() != &sym) continue;

    // --- Data components ---
    for (sema::SourceName const &compName : sym.get<sema::DerivedTypeDetails>().componentNames()) {
      auto it = child.find(compName);
      if (it == child.end()) continue;

      ir::FieldDecl fd;
      fd.name = compName.ToString();
      if (auto const *t = it->second.get().GetType())
        fd.type.name = t->AsFortran();
      info->fields.push_back(mi.intern(std::move(fd)));
    }

    // --- Type-bound procedures ---
    for (auto const &[bindingName, bindingRef] : child) {
      sema::Symbol const &bindingSym = bindingRef.get();
      if (!bindingSym.has<sema::ProcBindingDetails>()) continue;

      sema::Symbol const &actual = bindingSym.get<sema::ProcBindingDetails>().symbol();
      if (!actual.has<sema::SubprogramDetails>()) continue;
      auto const &sub = actual.get<sema::SubprogramDetails>();

      bool is_nopass = bindingSym.attrs().test(sema::Attr::NOPASS);

      ir::FunctionDecl fd;
      fd.simple_name      = bindingSym.name().ToString();
      fd.qualified_name   = module_name + "::" + actual.name().ToString();
      fd.parent_class_fqn = rec.fully_qualified_name;
      fd.is_method        = true;
      fd.is_static        = is_nopass;

      if (sub.isFunction()) {
        auto const *t       = sub.result().GetType();
        fd.return_type.name = t ? t->AsFortran() : "void";
      } else {
        fd.return_type.name = "void";
      }

      bool skip_pass = !is_nopass; // first dummy arg is the passed-object (self)
      for (sema::Symbol *arg : sub.dummyArgs()) {
        if (!arg) continue;
        if (skip_pass) { skip_pass = false; continue; }
        ir::ParamVarDecl p;
        p.name = arg->name().ToString();
        if (auto const *t = arg->GetType())
          p.type.name = t->AsFortran();
        fd.params.push_back(std::move(p));
      }

      fnt_ptr_t ptr = mi.intern(std::move(fd));
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
