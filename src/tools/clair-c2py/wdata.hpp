#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string_view>

#include "llvm/Support/Regex.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Frontend/CompilerInstance.h"

#include "../module_info.hpp"
#include "./configuration.hpp"
#include "clu/concept.hpp"
#include "utility/string_tools.hpp"

// ----------------------- wdata_t ------------------------------------
// Per-translation-unit working state: compiler instance, configuration, resolved concepts, and collected module data.
struct wdata_t {
  clang::CompilerInstance *ci;
  configuration config;

  std::optional<llvm::Regex> reject_names;

  // Concepts matched from c2py and h5 library (if present).
  struct {
    clu::concept_holder IsConvertiblePy2C;
    clu::concept_holder IsConvertibleC2Py;
    clu::concept_holder HasSerializeLikeBoost;
    clu::concept_holder HasHdf5;
    clu::concept_holder HasNonDeletedDefaultConstructor;
  } concepts;

  clang::VarTemplateDecl *is_wrapped_vtd    = nullptr; // c2py::is_wrapped<T>
  clang::ClassTemplateDecl *py_converter_ctd = nullptr; // c2py::py_converter<T>

  bool input_has_included_generated_cxx = false;
  std::vector<std::string> deps;

  // Table of fully-qualified type names -> .hxx filename, built by scanning the source directory.
  // Populated in the constructor; used in check_convertibility to suggest #include directives.
  std::map<str_t, str_t> wrapped_type_to_header;

  module_info_t module_info;

  // Clang-specific lookup used by concept checking and AST-level analysis in scan_classes.
  std::map<str_t, clang::CXXRecordDecl const *> clang_cls_by_fqn;

  wdata_t(clang::CompilerInstance *ci, configuration const &config);

  // Wrappers for corresponding methods of module_info.
  // Enables decoupling module_info and codegen from clang.
  void add_class_to_module(std::string_view name, clang::CXXRecordDecl const *cls);
  bool is_wrapped_in_module(clang::QualType ty) const;
};
