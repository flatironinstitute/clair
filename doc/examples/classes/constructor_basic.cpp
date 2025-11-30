// Basic constructor wrapping
#include <c2py/c2py.hpp>

namespace mylib {

  class point {
    double x_, y_;

    public:
    // Default constructor
    point() : x_(0.0), y_(0.0) {}

    // Constructor with parameters
    point(double x, double y) : x_(x), y_(y) {}

    double x() const { return x_; }
    double y() const { return y_; }
  };

} // namespace mylib
