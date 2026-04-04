#include "./decl_utils.hpp"

#include <fmt/format.h>
#include "utility/string_tools.hpp"
#include "clu/misc.hpp"

// ------------------------------

bool should_reject(clang::Decl const *decl, std::optional<llvm::Regex> const &reject_names, util::logger const *log) {
  auto *named_decl = llvm::dyn_cast<clang::NamedDecl>(decl);
  if (!named_decl) return true; // no name -> reject
  auto name = named_decl->getQualifiedNameAsString();
  // is annoted explicitely -> reject
  if (clu::has_annotation(named_decl, "c2py_ignore")) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "[C2PY_IGNORE]"));
    return true;
  }
  // matches the regex -> reject
  if (reject_names && reject_names->match(name)) {
    if (log) (*log)(fmt::format(R"RAW({0} [{1}])RAW", name, "[REJECT_NAMES]"));
    return true;
  }
  return false;
}

// ------------------------------

str_t get_python_name(clang::CXXRecordDecl const *cls) {
  if (auto rename = clu::get_annotation_value(cls, "c2py_rename")) return *rename;
  return util::camel_case(cls->getNameAsString());
}

str_t get_python_name(clang::FunctionDecl const *f) {
  str_t py_name = f->getNameAsString();
  if (auto rename = clu::get_annotation_value(f, "c2py_rename")) py_name = *rename;
  return py_name;
}
