// c2py_wrap_as_method.cpp

#include "c2py/c2py.hpp"

// Some class we want to wrap.
struct myclass {
  int x{};
};

// We want to add this function as a method to myclass in the Python bindings.
C2PY_WRAP_AS_METHOD int f(const myclass &m, int y) { return m.x * y; }
