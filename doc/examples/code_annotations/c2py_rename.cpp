// c2py_rename.cpp

#include "c2py/c2py.hpp"

// We want to rename f to g in the Python bindings.
C2PY_RENAME(g) int f(int x) { return x * 2; }
