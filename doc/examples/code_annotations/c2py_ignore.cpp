// c2py_ignore.cpp

#include "c2py/c2py.hpp"

// Some function we don't want to wrap.
C2PY_IGNORE int f(int x) { return x * 2; }

// Another function we want to wrap.
int g(int y) { return y * 3; }
