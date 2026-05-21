#include "module_info.hpp"
#include "tools/ir/types.hpp"
#include <algorithm>
#include <unordered_map>

// ------------------------------

// Deduplicate by IR signature (qualified_name + param types).
// When the same function appears multiple times (redeclarations), keep the one with defaults.
// fnt_info_t is trivially copyable (all raw pointers/bools), so direct copy is used throughout.
std::vector<fnt_info_t> make_unique_decls(std::vector<fnt_info_t> const &flist) {
  std::unordered_map<std::string, size_t> seen; // sig -> index in result
  std::vector<fnt_info_t> res;
  seen.reserve(flist.size());
  res.reserve(flist.size());

  for (auto const &f : flist) {
    if (not f.ptr) continue;
    auto sig        = f.ptr->qualified_name + "(" + f.ptr->param_types_str() + ")";
    auto [it, inserted] = seen.emplace(sig, res.size());
    if (inserted) {
      res.push_back(f);
    } else {
      bool new_has = std::ranges::any_of(f.ptr->params, [](auto const &p) { return p.has_default; });
      bool cur_has = std::ranges::any_of(res[it->second].ptr->params, [](auto const &p) { return p.has_default; });
      if (new_has and not cur_has) res[it->second] = f;
    }
  }
  return res;
}

// ------------------------------

fnt_ptr_t module_info_t::intern(ir::FunctionDecl f) {
  fnt_pool.push_back(std::make_unique<ir::FunctionDecl>(std::move(f)));
  return fnt_pool.back().get();
}

cls_ptr_t module_info_t::intern(ir::RecordDecl f) {
  rec_pool.push_back(std::make_unique<ir::RecordDecl>(std::move(f)));
  return rec_pool.back().get();
}

field_ptr_t module_info_t::intern(ir::FieldDecl f) {
  field_pool.push_back(std::make_unique<ir::FieldDecl>(std::move(f)));
  return field_pool.back().get();
}

enum_ptr_t module_info_t::intern(ir::EnumDecl f) {
  enum_pool.push_back(std::make_unique<ir::EnumDecl>(std::move(f)));
  return enum_pool.back().get();
}

// ------------------------------

void module_info_t::add_class(std::string_view name, ir::RecordDecl const &cls) {
  auto fqn = cls.getFQN();
  if (classes_fqn_to_info.contains(fqn)) return; // already registered
  auto *raw = intern(cls);
  long idx  = long(classes.size());
  classes.emplace_back(name, cls_info_t{.ptr = raw});
  classes_ptr_to_info[raw] = idx;
  classes_fqn_to_info[fqn] = idx;
}

// ------------------------------

cls_ptr_t module_info_t::get_wrapped_cls(std::string_view fqn) const {
  if (auto it = classes_fqn_to_info.find(fqn.data()); it != classes_fqn_to_info.end())
    return classes[it->second].second.ptr;
  return nullptr;
}

// ------------------------------

cls_info_t *module_info_t::get_wrapped_cls_info(std::string_view fqn) {
  if (auto it = classes_fqn_to_info.find(fqn.data()); it != classes_fqn_to_info.end())
    return &classes[it->second].second;
  return nullptr;
}

// ------------------------------

bool module_info_t::is_wrapped(std::string_view fqn) const { return get_wrapped_cls(fqn) != nullptr; }

// ------------------------------
