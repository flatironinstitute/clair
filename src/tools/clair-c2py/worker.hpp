#pragma once
#include "./configuration.hpp"
#include "./data.hpp"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"

struct worker_t {

  clang::CompilerInstance *ci;
  configuration config;

  std::string match_names, match_files; // Remove and just use config ? SAME
  std::optional<llvm::Regex> reject_names;

  clang::ConceptDecl const *IsConvertiblePy2C               = nullptr;
  clang::ConceptDecl const *IsConvertibleC2Py               = nullptr;
  clang::ConceptDecl const *HasSerializeLikeBoost           = nullptr;
  clang::ConceptDecl const *HasHdf5                         = nullptr;
  clang::ConceptDecl const *HasNonDeletedDefaultConstructor = nullptr;

  bool user_has_included_generated_cxx = false;

  module_info_t module_info;

  worker_t(clang::CompilerInstance *ci, configuration const &config);

  void run();

  /// Should the decl be ignored due to
  /// i) a c2py_ignore annotation
  /// ii) its qualified name matches the reject_names
  /// If log is present, it logs the rejection
  bool is_rejected(clang::Decl const *decl, util::logger const *log = nullptr);

  private:
  void scan_class_elements(cls_info_t &cls_info, cls_ptr_t cls);
  void scan_class_and_bases_elements();
  void prepare_methods();
  void remove_multiple_decl();
  void separate_properties();
  void check_convertibility();
};
