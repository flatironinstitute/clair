#pragma once
#include <regex>
#include <map>
#include <optional>
#include "utility/string_tools.hpp"
//#include "clu/misc.hpp"
#include "clang/AST/DeclCXX.h"

using cls_ptr_t = clang::CXXRecordDecl const *;
using fnt_ptr_t = clang::FunctionDecl const *;

// -----------------------------------------------------------

struct fnt_info_t {
  fnt_ptr_t ptr                            = nullptr;
  bool rewrite                             = false; // (as_method() ? false : true); //false; // do we need to rewrite it with a lambda (e.g. friend)
  clang::CXXRecordDecl const *parent_class = nullptr;
  //bool no_gil                              = clu::has_annotation(ptr, "c2py_nogil");
  [[nodiscard]] clang::CXXMethodDecl const *as_method() const { return llvm::dyn_cast_or_null<clang::CXXMethodDecl>(ptr); }
};

// Make the vector "unique", elimnating redondant declaration.
std::vector<fnt_info_t> make_unique(std::vector<fnt_info_t> const &flist);

// Analyse a c2py::dispatch declaration
// Used by worker and matchers (for methods and funtion resp.)
void analyse_dispatch(std::map<str_t, std::vector<fnt_info_t>> &fmap, clang::VarDecl const *decl);

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

  // Do we need to synthesize a constructor, as the class has only a {}
  // aggregate initialization
  [[nodiscard]] bool synthetize_init_from_pydict() const {
    return (ptr->isAggregate() and not ptr->hasTrivialDefaultConstructor() and (ptr->getNumBases() == 0));
  }
  [[nodiscard]] bool synthetize_dict_attribute() const { return synthetize_init_from_pydict(); }
};

// -----------------------------------------------------------

struct module_info_t {

  str_t module_name;
  str_t package_name;
  str_t sourcefile;
  str_t documentation;
  str_t ns;
  bool has_module_init = false;

  // Filters
  std::string match_names, match_files; // string used a regex in the AST Matchers directly
  std::optional<std::regex> reject_names;
  bool get_set_as_properties = false;

  std::map<str_t, std::vector<fnt_info_t>> functions; // vector not unique
  std::vector<std::pair<str_t, cls_info_t>> classes;  // index of cls_table. Must keep order of insertion to have base first
  std::vector<clang::EnumDecl const *> enums;         // all enums (including in classes)
};
