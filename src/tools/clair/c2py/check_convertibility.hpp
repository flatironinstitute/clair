#pragma once
#include "./wdata.hpp"

/// Check that all parameter types are convertible from Python to C++ and,
/// if test_return_type is true, that the return type is convertible from C++ to Python.
/// Emits clang diagnostics for each failing type.
/// Returns true if the function should be kept (all checks pass), false otherwise.
bool check_convertibility(clang::FunctionDecl const *f, wdata_t const &wd, bool test_return_type = true);
