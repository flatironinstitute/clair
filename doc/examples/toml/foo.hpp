// foo.hpp

#include <iostream>

namespace foo {
  inline void f1() { std::cout << "foo::f1" << std::endl; }
  inline void f2() { std::cout << "foo::f2" << std::endl; }
  inline void g1() { std::cout << "foo::g1" << std::endl; }
  inline void g2() { std::cout << "foo::g2" << std::endl; }

  class A {
    int x_;

    public:
    int x() const { return x_; }
    void set_x(int x) { x_ = x; }
  };
} // namespace foo