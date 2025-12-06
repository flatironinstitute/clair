#include "c2py/c2py.hpp"
#include <string>

template <typename T> T add(T a, T b) { return a + b; }

// clair-c2py will generate bindings for these EXPLICIT instantiations.
template int add(int a, int b);

template double add(double a, double b);

template std::string add(std::string a, std::string b);

#include "function_template.wrap.cxx"
