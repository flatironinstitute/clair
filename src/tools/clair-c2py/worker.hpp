#pragma once
#include "./configuration.hpp"
#include "./data.hpp"
#include "clang/Frontend/CompilerInstance.h"

struct worker_t {

  clang::CompilerInstance *ci;
  configuration config;

  std::optional<llvm::Regex> reject_names; // only regex, the other ones are used directly in the ASTMatcher

  // Concept matched from c2py, and h5 library (if present)
  // They are used with the Sema (clu::concept_check) to introspect types.
  // They are Matched in the ASTConsumer, and set to nullptr if not found.
  clang::ConceptDecl const *IsConvertiblePy2C               = nullptr;
  clang::ConceptDecl const *IsConvertibleC2Py               = nullptr;
  clang::ConceptDecl const *HasSerializeLikeBoost           = nullptr;
  clang::ConceptDecl const *HasHdf5                         = nullptr;
  clang::ConceptDecl const *HasNonDeletedDefaultConstructor = nullptr;

  // Preprocessor will detect if the input has included the generated cxx file
  // and store the result in this variable.
  bool input_has_included_generated_cxx = false;
  std::vector<std::string> deps; // dependencies collected by the preprocessor

  // All the information about the module including the classes, methods, etc.
  module_info_t module_info;

  // Constructor
  worker_t(clang::CompilerInstance *ci, configuration const &config);

  // To be called by the ASTConsumer after the matchers have been run
  // to finalize the classes, and some checks
  void run();

  // A function for the matchers.
  // Should the decl be ignored due to
  // i) a c2py_ignore annotation
  // ii) its qualified name matches the reject_names
  // If log is present, it logs the rejection
  bool is_rejected(clang::Decl const *decl, util::logger const *log = nullptr);

  /// Check if the function parameters and return type are convertible
  bool check_convertibility(clang::FunctionDecl const *f, bool test_return_type = true) const;

  private:
  void analyze_one_method(clang::FunctionDecl const *f, cls_info_t &cls_info, cls_ptr_t cls);
  void scan_class_elements(cls_info_t &cls_info, cls_ptr_t cls);
  void scan_class_and_bases_elements(cls_info_t &cls_info);
  void separate_properties(cls_info_t &cls_info);
};
