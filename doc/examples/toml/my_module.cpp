// my_module.cpp

#include <c2py/c2py.hpp>
#include "foo.hpp"
#include "bar.hpp"
#include <iostream>

namespace foo {
  void f1() { std::cout << "foo::f1" << std::endl; }
  void f2() { std::cout << "foo::f2" << std::endl; }
  void g1() { std::cout << "foo::g1" << std::endl; }
  void g2() { std::cout << "foo::g2" << std::endl; }
} // namespace foo

namespace bar {
  void h1() { std::cout << "bar::h1" << std::endl; }
  void h2() { std::cout << "bar::h2" << std::endl; }
} // namespace bar
