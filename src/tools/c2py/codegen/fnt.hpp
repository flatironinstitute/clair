#pragma once
#include "../data.hpp"

// ========== A few helper function to extract params of a function ===========

// e.g. f(A a, B b = 2) --->   a,b
str_t fnt_params(fnt_ptr_t f);

// same with tpl parameters
str_t fnt_tparams(fnt_ptr_t f);

// e.g. f(A a, B b = 2) --->   A, B
str_t fnt_paramtypes(fnt_ptr_t f);

// A , separated list of parameters names, with default
// e.g. f(A a, B b = 2) --->   "a", "b"_a = 2
str_t fnt_params_with_default(clang::FunctionDecl const *f);

// a small utility to get a comma if s is not empty
inline char comma_if(str_t const &s) { return (s.empty() ? ' ' : ','); }

// ========== Code generation for a function =======================

namespace codegen {

  void write_dispatch(std::ostream &code, std::ostream &table, std::ostream &doc, std::string const &pyname, std::vector<fnt_info_t> const &flist,
                      clang::CXXRecordDecl const *parent_class, bool enforce_method);

  void write_dispatch_constructors(std::ostream &code, std::string const &cls_cpp_name, std::vector<fnt_info_t> const &flist);

} // namespace codegen
