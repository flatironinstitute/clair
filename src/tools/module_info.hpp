#pragma once
#include "ir/types.hpp"

// Non-owning observer aliases: raw const pointers into IR node pools owned by module_info_t.
using cls_ptr_t   = ir::RecordDecl const *;
using fnt_ptr_t   = ir::FunctionDecl const *;
using field_ptr_t = ir::FieldDecl const *;
using enum_ptr_t  = ir::EnumDecl const *;

// -----------------------------------------------------------
// Operator kind: arithmetic, comparison, and unary
enum class OpKind { Add, Sub, Mul, Div, LShift, Eq, Ne, Lt, Gt, Le, Ge, Neg, Pos, IAdd, ISub, IMul, IDiv };

// ----------------------- fnt_info_t ------------------------------------
// Carries a function/method declaration pointer and wrapping metadata.
// Trivially copyable: all members are raw pointers or bools.
struct fnt_info_t {
  fnt_ptr_t ptr             = nullptr;
  bool rewrite              = true;
  cls_ptr_t parent_class    = nullptr;
  bool is_inherited_method  = false;
};

/// Deduplicate a list of functions, keeping the best redeclaration per group.
std::vector<fnt_info_t> make_unique_decls(std::vector<fnt_info_t> const &flist);

// -------------------  Serialization method ----------------------------------------
enum class Serialization { None, Tuple, H5, Repr };

// ----------------------- cls_info_t ------------------------------------
// Collects all wrapping data for a C++ class: methods, constructors, fields, operators, and properties.
struct cls_info_t {
  cls_ptr_t ptr  = nullptr;
  cls_ptr_t base = nullptr;

  std::map<str_t, std::vector<fnt_info_t>> methods = {};
  std::vector<fnt_info_t> constructors             = {};

  std::vector<field_ptr_t> fields = {};

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
  std::map<str_t, property> properties = {};

  std::map<OpKind, std::vector<std::vector<ir::QualType>>> operators = {};

  bool synthetize_init_from_pydict() const { return ptr and ptr->synthetize_init_from_pydict(); }
};

// ----------------------- module_info_t ------------------------------------
// Aggregates all wrapping data for a Python module: functions, classes, enums, and source metadata.
struct module_info_t {

  str_t module_name;
  str_t package_name;
  str_t sourcefile;           // full path name of the source file
  str_t sourcefile_full_stem;
  str_t documentation;
  str_t module_init_fqn; // FQN of the module_init function, empty if none

  std::map<str_t, std::vector<fnt_info_t>> functions;
  std::vector<enum_ptr_t> enums;

  std::vector<std::pair<str_t, cls_info_t>> classes; // Must keep order of insertion to have base first
  std::map<cls_ptr_t, long> classes_ptr_to_info;     // raw ptr → index
  std::map<str_t, long> classes_fqn_to_info;         // FQN string → index

  // Ownership pools for IR nodes; all raw pointer aliases above point into these.
  std::vector<std::unique_ptr<ir::FunctionDecl>> fnt_pool;
  std::vector<std::unique_ptr<ir::RecordDecl>>   rec_pool;
  std::vector<std::unique_ptr<ir::FieldDecl>>    field_pool;
  std::vector<std::unique_ptr<ir::EnumDecl>>     enum_pool;

  // Intern an IR node into the appropriate pool and return a non-owning const pointer.
  fnt_ptr_t   intern(ir::FunctionDecl);
  cls_ptr_t   intern(ir::RecordDecl);
  field_ptr_t intern(ir::FieldDecl);
  enum_ptr_t  intern(ir::EnumDecl);

  // Add a class to the module; silently ignored if already registered.
  void add_class(std::string_view name, ir::RecordDecl const &cls);

  // Return the wrapped IR class pointer (non-owning) for a type (by fqn), or nullptr if not wrapped.
  cls_ptr_t get_wrapped_cls(std::string_view fqn) const;

  // Return a pointer to the cls_info_t for a wrapped type (by fqn), or nullptr.
  cls_info_t *get_wrapped_cls_info(std::string_view fqn);

  // Return true if the type (by fqn) corresponds to a wrapped class.
  bool is_wrapped(std::string_view fqn) const;
};
