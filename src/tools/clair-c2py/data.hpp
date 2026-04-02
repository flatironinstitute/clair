#pragma once
#include <regex>
#include <map>
#include <optional>
#include "utility/string_tools.hpp"
#include "utility/logger.hpp"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/ADT/STLExtras.h"

using cls_ptr_t = clang::CXXRecordDecl const *;
using fnt_ptr_t = clang::FunctionDecl const *;

// -----------------------------------------------------------
// Operator kind: arithmetic, comparison, and unary
enum class OpKind { Add, Sub, Mul, Div, LShift, Eq, Ne, Lt, Gt, Le, Ge, Neg, Pos, IAdd, ISub, IMul, IDiv };

// -----------------------------------------------------------

// // Template specializations can use a function pointer (&f<targs>) unless
// // the template has a parameter pack, which c2py's dispatcher can't handle.
// inline bool fnt_needs_rewrite(fnt_ptr_t f) {
//   if (!f) return true;
//   auto *info = f->getTemplateSpecializationInfo();
//   if (!info) return true; // non-template: rewrite
//   // Check if the template declaration has a parameter pack
//   for (auto *p : info->getTemplate()->getTemplateParameters()->asArray())
//     if (p->isParameterPack()) return true;
//   return false;
// }

struct fnt_info_t {
  fnt_ptr_t ptr          = nullptr;
  bool rewrite           = true; // fnt_needs_rewrite(ptr);
  cls_ptr_t parent_class = nullptr;
  [[nodiscard]] clang::CXXMethodDecl const *as_method() const { return llvm::dyn_cast_or_null<clang::CXXMethodDecl>(ptr); }
};

// -----------------------------------------------------------
// Serialization method
enum class Serialization { None, Tuple, H5, Repr };

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
  std::map<str_t, property> properties = {}; // pyname -> list of C++ overloads

  // operators: op -> list of signatures (each signature = full argument type list)
  // e.g. Add -> {{A, A}, {A, int}}, Neg -> {{A}}
  std::map<OpKind, std::vector<std::vector<clang::QualType>>> operators = {};

  // Do we need to synthesize a constructor, as the class has only a {}
  // aggregate initialization
  bool synthetize_init_from_pydict() const { return (ptr->isAggregate() and (ptr->getNumBases() == 0)); }

  bool synthetize_dict_attribute() const { return synthetize_init_from_pydict(); }
};

// -----------------------------------------------------------

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

  void add_class(std::string_view name, clang::CXXRecordDecl const *cls) {
    if (classes_ptr_to_info.contains(cls)) return; // already wrapped.
    classes.emplace_back(name, cls_info_t{.ptr = cls});
    classes_ptr_to_info[cls] = long(classes.size() - 1); // index in classes
  }

  cls_ptr_t get_wrapped_cls(clang::QualType ty) const {
    clang::CXXRecordDecl const *cls = ty->getAsCXXRecordDecl();
    if (!cls) cls = ty->getPointeeCXXRecordDecl();
    return (cls and classes_ptr_to_info.contains(cls)) ? cls : nullptr;
  }

  cls_info_t *get_wrapped_cls_info(clang::QualType ty) {
    auto cls = get_wrapped_cls(ty);
    return cls ? &classes[classes_ptr_to_info.at(cls)].second : nullptr;
  }

  bool is_wrapped(clang::QualType ty) const { return get_wrapped_cls(ty) != nullptr; }
};
