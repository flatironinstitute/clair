#include "./wdata.hpp"

#include <filesystem>
#include "llvm/ADT/DenseSet.h"

// ------------------------------

// Among all redeclarations of f, pick the best one:
// 1. Prefer a redecl with default arguments (at most one exists per C++ rules)
// 2. Otherwise prefer a redecl where all parameters are named
// 3. Fall back to the most recent redecl
static const clang::FunctionDecl *best_redecl(const clang::FunctionDecl *f) {
  const clang::FunctionDecl *with_names = nullptr;
  for (auto *redecl : f->redecls()) {
    auto *r = llvm::dyn_cast<clang::FunctionDecl>(redecl);
    if (not r) continue;
    if (llvm::any_of(r->parameters(), [](auto *p) { return p->hasDefaultArg(); })) return r;
    if (not with_names and llvm::all_of(r->parameters(), [](auto *p) { return !p->getName().empty(); })) with_names = r;
  }
  return with_names ? with_names : f->getMostRecentDecl();
}

// ------------------------------

std::vector<fnt_info_t> make_unique_decls(std::vector<fnt_info_t> const &flist) {
  llvm::DenseSet<const clang::FunctionDecl *> seen; // LLVM recommended replacement of std::set
  std::vector<fnt_info_t> res;
  seen.reserve(flist.size());
  res.reserve(flist.size());

  for (const auto &f : flist) {
    if (seen.insert(f.ptr->getMostRecentDecl()).second) {
      auto *best = best_redecl(f.ptr);
      res.push_back({.ptr = best, .rewrite = f.rewrite, .parent_class = f.parent_class});
    }
  }

  return res;
}

// ------------------------------

void module_info_t::add_class(std::string_view name, clang::CXXRecordDecl const *cls) {
  auto *key = cls->getCanonicalDecl();
  if (classes_ptr_to_info.contains(key)) return; // already registered
  classes.emplace_back(name, cls_info_t{.ptr = cls});
  classes_ptr_to_info[key] = long(classes.size() - 1);
}

// ------------------------------

cls_ptr_t module_info_t::get_wrapped_cls(clang::QualType ty) const {
  clang::CXXRecordDecl const *cls = ty->getAsCXXRecordDecl();
  if (!cls) cls = ty->getPointeeCXXRecordDecl();
  if (cls) cls = cls->getCanonicalDecl();
  return (cls and classes_ptr_to_info.contains(cls)) ? cls : nullptr;
}

// ------------------------------

cls_info_t *module_info_t::get_wrapped_cls_info(clang::QualType ty) {
  auto cls = get_wrapped_cls(ty);
  return cls ? &classes[classes_ptr_to_info.at(cls)].second : nullptr;
}

// ------------------------------

bool module_info_t::is_wrapped(clang::QualType ty) const { return get_wrapped_cls(ty) != nullptr; }

// ------------------------------

wdata_t::wdata_t(clang::CompilerInstance *ci, configuration const &config) : ci{ci}, config{config} {

  auto p                           = std::filesystem::absolute(ci->getFrontendOpts().Inputs[0].getFile().str());
  module_info.sourcefile           = str_t{p.string()};
  module_info.module_name          = str_t{p.stem()};
  module_info.sourcefile_full_stem = p.parent_path() / p.stem();
  module_info.package_name         = config.package_name;
  module_info.documentation        = config.documentation;

  // Validity of the regex is checked in the configuration constructor
  if (not config.reject_names.empty()) this->reject_names = llvm::Regex(config.reject_names);
}
