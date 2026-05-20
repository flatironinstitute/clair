#pragma once
#include "../../ir/types.hpp"
#include "../wdata.hpp"

// ========== IR-based helper functions to extract params of a function ===========

// e.g. f(A a, B b = 2) --->   a,b
str_t fnt_params(ir::FunctionDecl const &f);

// same with template parameters
str_t fnt_tparams(ir::FunctionDecl const &f);

// e.g. f(A a, B b = 2) --->   A, B
str_t fnt_paramtypes(ir::FunctionDecl const &f);

// e.g. f(A a, B b = 2) --->   A a, B b
str_t fnt_param_with_types(ir::FunctionDecl const &f);

// A , separated list of parameters names, with defaults
// e.g. f(A a, B b = 2) --->  "a", "b"_a = 2
str_t fnt_params_with_default(ir::FunctionDecl const &f);

// Returns ',' if s is non-empty, ' ' otherwise (used to conditionally separate kw-arg lists)
inline char comma_if(str_t const &s) { return (s.empty() ? ' ' : ','); }
