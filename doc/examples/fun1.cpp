#include <c2py/c2py.hpp>

int f(int x) { return -x; }
int f(int x, int y) { return x + y; }
std::string f(std::string const &s) { return s + s; }

#include "fun1.wrap.cxx"
