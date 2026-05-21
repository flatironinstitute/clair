#include "fnt_tools.hpp"

// All implementations delegate to the IR-level helper methods,
// keeping codegen decoupled from the clang AST.

str_t fnt_params(ir::FunctionDecl const &f)             { return f.param_names_str(); }
str_t fnt_tparams(ir::FunctionDecl const &f)            { return f.targs_str(); }
str_t fnt_paramtypes(ir::FunctionDecl const &f)         { return f.param_types_str(); }
str_t fnt_param_with_types(ir::FunctionDecl const &f)   { return f.params_with_types_str(); }
str_t fnt_params_with_default(ir::FunctionDecl const &f){ return f.params_with_defaults_str(); }
