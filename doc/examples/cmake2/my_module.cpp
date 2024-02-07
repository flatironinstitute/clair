#include "c2py/c2py.hpp"

/// A wonderful little class
class my_class{
  int a, b;
  public:
    my_class(int a_, int b_) : a(a_), b(b_) {}
    int f(int u) const { return u + a;}
};

