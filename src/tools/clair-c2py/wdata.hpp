#pragma once
#include <regex>
#include <map>
#include <optional>

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Regex.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/Frontend/CompilerInstance.h"

#include "./configuration.hpp"
#include "clu/concept.hpp"
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"

using cls_ptr_t = clang::CXXRecordDecl const *;
using fnt_ptr_t = clang::FunctionDecl const *;

// -----------------------------------------------------------
// Operator kind: arithmetic, comparison, and unary
enum class OpKind { Add, Sub, Mul, Div, LShift, Eq, Ne, Lt, Gt, Le, Ge, Neg, Pos, IAdd, ISub, IMul, IDiv };

// ----------------------- fnt_info_t ------------------------------------
// Carries a function/method declaration pointer and wrapping metadata.
struct fnt_info_t {
  fnt_ptr_t ptr          = nullptr;
  bool rewrite           = true;
  cls_ptr_t parent_class = nullptr;
  [[nodiscard]] clang::CXXMethodDecl const *as_method() const { return llvm::dyn_cast_or_null<clang::CXXMethodDecl>(ptr); }
};

/// Deduplicate a list of functions, keeping the best redeclaration per group.
std::vector<fnt_info_t> make_unique_decls(std::vector<fnt_info_t> const &flist);

// -------------------  Serialization method ----------------------------------------
enum class Serialization { None, Tuple, H5, Repr };

// ----------------------- cls_info_t ------------------------------------
// Collects all wrapping data for a C++ class: methods, constructors, fields, operators, and properties.
struct cls_info_t {
  cls_ptr_t ptr;
  cls_ptr_t base                                   = nullptr;
  std::map<str_t, std::vector<fnt_info_t>> methods = {}; // pyname -> list of C++ overloads
  std::vector<fnt_info_t> constructors             = {};
  std::vector<clang::FieldDecl *> fields           = {};
  std::vector<fnt_info_t> getitems                 = {};
  std::vector<fnt_info_t> setitems                 = {};
  bool has_size_method                             = false;
  bool has_iterator                                = false;
  bool has_hdf5                                    = false;
  Serialization serialization                      = Serialization::None;

  struct property {
    fnt_info_t getter;
    std::vector<fnt_info_t> setters;
  };
  std::map<str_t, property> properties = {}; // pyname -> property (getter + setters)

  // operators: op -> list of signatures (each signature = full argument type list)
  // e.g. Add -> {{A, A}, {A, int}}, Neg -> {{A}}
  std::map<OpKind, std::vector<std::vector<clang::QualType>>> operators = {};

  // Do we need to synthesize a constructor, as the class has only a {}
  // aggregate initialization
  bool synthetize_init_from_pydict() const { return (ptr->isAggregate() and (ptr->getNumBases() == 0)); }
};

// ----------------------- module_info_t ------------------------------------
// Aggregates all wrapping data for a Python module: functions, classes, enums, and source metadata.
struct module_info_t {

  str_t module_name;
  str_t package_name;
  str_t sourcefile;           // full path name of the source file, e.g. "/some/path/to/my_module.cpp"
  str_t sourcefile_full_stem; //  e.g. "/some/path/to/my_module"
  str_t documentation;
  clang::FunctionDecl const *module_init = nullptr;

  std::map<str_t, std::vector<fnt_info_t>> functions; // vector not unique
  std::vector<clang::EnumDecl const *> enums;         // all enums (including in classes)

  std::vector<std::pair<str_t, cls_info_t>> classes; // index of cls_table. Must keep order of insertion to have base first
  std::map<cls_ptr_t, long> classes_ptr_to_info;     // reverse search table

  // Add a class to the module; silently ignored if already registered.
  void add_class(std::string_view name, clang::CXXRecordDecl const *cls);

  // Return the wrapped class pointer for a type, or nullptr if not wrapped.
  cls_ptr_t get_wrapped_cls(clang::QualType ty) const;

  // Return a pointer to the cls_info_t for a wrapped type, or nullptr.
  cls_info_t *get_wrapped_cls_info(clang::QualType ty);

  // Return true if the type corresponds to a wrapped class or a wrapped enum.
  bool is_wrapped(clang::QualType ty) const;
};

// ----------------------- wdata_t ------------------------------------
// Per-translation-unit working state: compiler instance, configuration, resolved concepts, and collected module data.
struct wdata_t {
  clang::CompilerInstance *ci;
  configuration config;

  std::optional<llvm::Regex> reject_names; // only regex, the other ones are used directly in the ASTMatcher

  // Concepts matched from c2py and h5 library (if present).
  // Resolved by the ASTConsumer; concept_holder::is_satisfied_by(...) returns false if not found.
  struct {
    clu::concept_holder IsConvertiblePy2C;
    clu::concept_holder IsConvertibleC2Py;
    clu::concept_holder HasSerializeLikeBoost;
    clu::concept_holder HasHdf5;
    clu::concept_holder HasNonDeletedDefaultConstructor;
  } concepts;

  clang::VarTemplateDecl *is_wrapped_vtd         = nullptr; // c2py::is_wrapped<T>
  clang::ClassTemplateDecl *py_converter_ctd = nullptr; // c2py::py_converter<T>

  // Preprocessor will detect if the input has included the generated cxx file
  // and store the result in this variable.
  bool input_has_included_generated_cxx = false;
  std::vector<std::string> deps; // dependencies collected by the preprocessor

  // Table of fully-qualified type names -> .hxx filename, built by scanning the source directory.
  // Populated in the constructor; used in check_convertibility to suggest #include directives.
  std::map<str_t, str_t> wrapped_type_to_header;

  // All the information about the module including the classes, methods, etc.
  module_info_t module_info;

  wdata_t(clang::CompilerInstance *ci, configuration const &config);
};
