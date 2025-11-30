// Instance methods wrapping
#include <c2py/c2py.hpp>

namespace mylib {

  class rectangle {
    double width_, height_;

    public:
    rectangle(double w, double h) : width_(w), height_(h) {}

    // Const method
    double area() const { return width_ * height_; }

    // Non-const method
    void scale(double factor) {
      width_ *= factor;
      height_ *= factor;
    }

    // Overloaded methods
    void resize(double factor) { scale(factor); }
    void resize(double w, double h) {
      width_  = w;
      height_ = h;
    }
  };

} // namespace mylib
