#include <c2py/c2py.hpp>
class point {
  double x_, y_;

  public:
  point() : point(0, 0) {}                    // Default constructor
  point(double x, double y) : x_(x), y_(y) {} // Constructor with parameters

  double x() const { return x_; }
  double y() const { return y_; }
};

#include "constructor_basic.wrap.cxx"
