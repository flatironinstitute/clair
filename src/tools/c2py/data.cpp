#include <regex>
#include <numeric>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include <itertools/itertools.hpp>
#include <clang/AST/QualTypeNames.h>
#include "clang/AST/DeclTemplate.h"
#include <clang/AST/Attr.h>
#include "utility/string_tools.hpp"
#include "utility/macros.hpp"
#include "clu/misc.hpp"
#include "./data.hpp"

// -------------------------------------

// get a set of indices giving all decls, once only.
std::vector<fnt_info_t> make_unique(std::vector<fnt_info_t> const &flist) {

  // mrd : most recent declaration
  std::vector<fnt_ptr_t> f_mrd(flist.size());
  auto get_mrd = [](fnt_ptr_t f) { return (f ? llvm::dyn_cast_or_null<clang::FunctionDecl>(f->getMostRecentDecl()) : nullptr); };
  for (int i = 0; i < flist.size(); ++i) f_mrd[i] = get_mrd(flist[i].ptr);

  // redeclaration of f is not found.
  std::vector<int> f_idx(flist.size());
  std::iota(f_idx.begin(), f_idx.end(), 0);
  std::sort(f_idx.begin(), f_idx.end(), [&f_mrd](auto i, auto j) { return long(f_mrd[i]) < long(f_mrd[j]); });
  auto last = std::unique(f_idx.begin(), f_idx.end(), [&f_mrd](auto i, auto j) { return long(f_mrd[i]) == long(f_mrd[j]); });
  f_idx.erase(last, f_idx.end());
  std::sort(f_idx.begin(), f_idx.end());
  std::vector<fnt_info_t> res;
  res.reserve(f_idx.size());
  for (int i : f_idx) res.push_back(flist[i]);
  return res;
}

// -------------------------------------

void analyse_dispatch(std::map<str_t, std::vector<fnt_info_t>> &fmap, clang::VarDecl const *decl) {
  if (auto *cls = decl->getType()->getAsCXXRecordDecl(); cls and cls->getQualifiedNameAsString() == "c2py::dispatch_t") {

    if (auto spe = llvm::dyn_cast_or_null<clang::ClassTemplateSpecializationDecl>(cls)) {
      for (auto const &targ : spe->getTemplateInstantiationArgs()[0].pack_elements()) {
        if (auto *f = llvm::dyn_cast_or_null<clang::FunctionDecl>(targ.getAsDecl())) {
          auto fname = str_t{decl->getName()};
          fmap[fname].push_back(fnt_info_t{f, false});
        } else
          clu::emit_error(targ.getAsDecl(), "c2py: c2py::dispatch<...> takes only function pointers");
      }
    }
  } else
    clu::emit_error(decl, "c2py: only c2py::dispatch<...> declaration is authorized here");
}

// -----------------------------
bool is_rejected(clang::Decl const *decl, std::optional<std::regex> const &reject_regex, util::logger const *log) {
  auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
  if (!named_decl) return true; // no name -> reject
  auto name = named_decl->getQualifiedNameAsString();
  // is annoted explicitely -> reject
  if (clu::has_annotation(named_decl, "c2py_ignore")) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "C2PY_IGNORE"));
    return true;
  }
  // matches the regex -> reject
  if (reject_regex and std::regex_match(name, reject_regex.value())) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "reject_names"));
    return true;
  }
  return false;
}
