#include "../module_info.hpp"
#include <optional>
#include <string>
#include <vector>

#include "llvm/Support/Regex.h"
#include "flang/Frontend/CompilerInstance.h"

// ----------------------- wdata_t ------------------------------------
// Per-translation-unit working state: compiler instance, configuration, resolved concepts, and collected module data.
struct wdata_t {
  Fortran::frontend::CompilerInstance *ci;
  // configuration config;

  std::optional<llvm::Regex> reject_names;

  // Concepts matched from c2py and h5 library (if present).
  // struct {
  //   clu::concept_holder IsConvertiblePy2C;
  //   clu::concept_holder IsConvertibleC2Py;
  //   clu::concept_holder HasSerializeLikeBoost;
  //   clu::concept_holder HasHdf5;
  //   clu::concept_holder HasNonDeletedDefaultConstructor;
  // } concepts;

  bool input_has_included_generated_cxx = false;
  std::vector<std::string> deps;

  // Table of fully-qualified type names -> .hxx filename, built by scanning the source directory.
  // Populated in the constructor; used in check_convertibility to suggest #include directives.
  // std::map<str_t, str_t> wrapped_type_to_header;

  module_info_t module_info;

  wdata_t(Fortran::frontend::CompilerInstance *ci /*, configuration const &config */);

  // Wrappers for corresponding methods of module_info.
  // Enables decoupling module_info and codegen from clang.
  // void add_derived_type_to_module(std::string_view name, Fortran::semantics::Scope const *ty); // (?)
  // bool is_wrapped_in_module(Fortran::semantics::Symbol const *s) const;
};
