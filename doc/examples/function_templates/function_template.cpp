#include "c2py/c2py.hpp"
#include <string>

// A function template that adds two values of the same type.
template <typename T> T add(T a, T b) { return a + b; }

// Explicit instantiation for int, double, and std::string types.
template int add<int>(int a, int b);
template double add<double>(double a, double b);
template std::string add<std::string>(std::string a, std::string b);
