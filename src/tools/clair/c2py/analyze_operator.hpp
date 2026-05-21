#pragma once
#include "./wdata.hpp"

/// Map a C++ operator (free or member) to an OpKind and register it
/// with the cls_info of its first (or second) wrapped operand type.
void analyze_operator(clang::FunctionDecl const *f, wdata_t &wd);
