// Arithmetic operators
#include <c2py/c2py.hpp>

namespace mylib {

  class vector {
    public:
    double x, y;

    vector(double x, double y) : x(x), y(y) {}

    // Arithmetic operators
    vector operator+(vector const &other) const { return vector(x + other.x, y + other.y); }

    vector operator-(vector const &other) const { return vector(x - other.x, y - other.y); }

    vector operator*(double scalar) const { return vector(x * scalar, y * scalar); }

    vector &operator+=(vector const &other) {
      x += other.x;
      y += other.y;
      return *this;
    }
  };

} // namespace mylib
