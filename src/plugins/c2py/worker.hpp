#pragma once
#include "./data.hpp"
#include "clang/Frontend/CompilerInstance.h"
struct worker_t {

  clang::CompilerInstance *ci;
  clang::ConceptDecl const *IsConvertiblePy2C               = nullptr;
  clang::ConceptDecl const *IsConvertibleC2Py               = nullptr;
  clang::ConceptDecl const *force_instantiation_add_methods = nullptr;
  clang::ConceptDecl const *HasSerializeLikeBoost           = nullptr;
  clang::ConceptDecl const *HasHdf5                         = nullptr;

  module_info_t module_info;
  clang::ClassTemplateDecl const *add_methods_to = nullptr;

  worker_t(clang::CompilerInstance *ci);

  void run();

  private:
  void get_additional_methods();
  void scan_class_and_bases_elements();
  void prepare_methods();
  void remove_multiple_decl();
  void separate_properties();
  void check_convertibility();
};
