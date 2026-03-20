#pragma once
#include "./configuration.hpp"
#include "./data.hpp"
#include "clang/Frontend/CompilerInstance.h"

struct worker_t {

  clang::CompilerInstance *ci;
  configuration config;

  std::optional<llvm::Regex> reject_names; // only regex, the other ones are used directly in the ASTMatcher

  // Concepts matched from c2py and h5 library (if present).
  // Resolved by the ASTConsumer; nullptr if not found.
  struct {
    clang::ConceptDecl const *IsConvertiblePy2C               = nullptr;
    clang::ConceptDecl const *IsConvertibleC2Py               = nullptr;
    clang::ConceptDecl const *HasSerializeLikeBoost           = nullptr;
    clang::ConceptDecl const *HasHdf5                         = nullptr;
    clang::ConceptDecl const *HasNonDeletedDefaultConstructor = nullptr;
  } concepts;

  // Preprocessor will detect if the input has included the generated cxx file
  // and store the result in this variable.
  bool input_has_included_generated_cxx = false;
  std::vector<std::string> deps; // dependencies collected by the preprocessor

  // All the information about the module including the classes, methods, etc.
  module_info_t module_info;

  worker_t(clang::CompilerInstance *ci, configuration const &config);

  /// Finalize classes after AST matchers have run: scan members, detect
  /// serialization, deduplicate methods, and check constructibility.
  void run();

  /// Check whether decl should be skipped due to a c2py_ignore annotation
  /// or a qualified name matching reject_names. Logs the reason if log is provided.
  bool is_rejected(clang::Decl const *decl, util::logger const *log = nullptr);

  /// Check that all parameter types are convertible from Python to C++ and,
  /// if test_return_type is true, that the return type is convertible from C++ to Python.
  /// Emits clang diagnostics for each failing type. Returns false if any check fails.
  bool check_convertibility(clang::FunctionDecl const *f, bool test_return_type = true) const;

  /// Return the Python name for a class, honoring c2py_rename or falling back to camelCase.
  str_t get_python_name(clang::CXXRecordDecl const *cls) const;
  /// Return the Python name for a function, honoring c2py_rename or keeping the C++ name.
  str_t get_python_name(clang::FunctionDecl const *f) const;

  /// Map a C++ operator (free or member) to an OpKind and register it
  /// with the cls_info of its first (or second) wrapped operand type.
  void analyze_operator(clang::FunctionDecl const *f);

  private:
  /// Classify a single method or constructor: dispatch to constructors,
  /// operator[], operator(), properties, iterators, or generic methods.
  void analyze_one_method(clang::FunctionDecl const *f, cls_info_t &cls_info, cls_ptr_t cls);
  /// Iterate over public declarations of cls and analyze methods, templates, and fields.
  void scan_class_elements(cls_info_t &cls_info, cls_ptr_t cls);
  /// Scan the class and its non-wrapped base classes, detect serialization
  /// and HDF5 support, then deduplicate and remove const overloads.
  void scan_class_and_bases_elements(cls_info_t &cls_info);
};
