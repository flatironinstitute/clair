// Copy semantics
#include <c2py/c2py.hpp>
#include <vector>

namespace mylib {

  class data {
    int value_;
    std::vector<double> buffer_;

    public:
    data(int value) : value_(value), buffer_(100, 0.0) {}

    // Copy constructor
    data(data const &other) : value_(other.value_), buffer_(other.buffer_) {}

    // Copy assignment
    data &operator=(data const &other) {
      if (this != &other) {
        value_  = other.value_;
        buffer_ = other.buffer_;
      }
      return *this;
    }

    int value() const { return value_; }
  };

} // namespace mylib
