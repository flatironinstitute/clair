#include "c2py/c2py.hpp"
#include <string>

// A class template.
template <typename T> struct adder {
  T a;
  adder(T a) : a(a) {}
  T add(T b) { return a + b; }
};

namespace c2py_module {
  // Using declarations to instantiate the class template for int, double and std::string.
  using int_adder    = adder<int>;
  using double_adder = adder<double>;
  using string_adder = adder<std::string>;
} // namespace c2py_module
