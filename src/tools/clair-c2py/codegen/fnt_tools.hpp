#pragma once
#include "../wdata.hpp"

// ========== A few helper functions to extract params of a function ===========

// e.g. f(A a, B b = 2) --->   a,b
str_t fnt_params(fnt_ptr_t f);

// same with tpl parameters
str_t fnt_tparams(fnt_ptr_t f);

// e.g. f(A a, B b = 2) --->   A, B
str_t fnt_paramtypes(fnt_ptr_t f);

// e.g. f(A a, B b = 2) --->   A a, B b
str_t fnt_param_with_types(fnt_ptr_t f);

// A , separated list of parameters names, with default
// e.g. f(A a, B b = 2) --->   "a", "b"_a = 2
str_t fnt_params_with_default(clang::FunctionDecl const *f);

// a small utility to get a comma if s is not empty
inline char comma_if(str_t const &s) { return (s.empty() ? ' ' : ','); }
