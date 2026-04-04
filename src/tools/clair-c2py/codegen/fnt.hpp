#pragma once
#include "fnt_tools.hpp"

// ========== Code generation for a function =======================

namespace codegen {

  void write_dispatch(std::ostream &code, std::ostream &table, std::ostream &doc, std::string const &pyname, std::vector<fnt_info_t> const &flist,
                      clang::CXXRecordDecl const *parent_class, bool enforce_method, std::string const &cls_alias = "");

  void write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::string const &cls_log_name,
                                   std::vector<fnt_info_t> const &flist);

} // namespace codegen
