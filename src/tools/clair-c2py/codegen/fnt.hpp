#pragma once
#include "fnt_tools.hpp"

// ========== Code generation for a function =======================

namespace codegen {

  // Generate a dispatcher for a set of function overloads (pyname).
  // ir_parent: IR of the class being wrapped (nullptr for free functions).
  // cls_alias: the local C++ alias for the class (e.g. _c2py_cls_0).
  void write_dispatch(std::ostream &code, std::ostream &table, std::ostream &doc, std::string const &pyname,
                      std::vector<fnt_info_t> const &flist, ir::RecordDecl const *ir_parent, bool enforce_method,
                      std::string const &cls_alias = "");

  void write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::string const &cls_log_name,
                                   std::vector<fnt_info_t> const &flist);

} // namespace codegen
