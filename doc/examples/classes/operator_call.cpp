// operator() wrapping
#include <c2py/c2py.hpp>

namespace mylib {

  class multiplier {
    int factor_;

    public:
    multiplier(int factor) : factor_(factor) {}

    // Function call operator
    int operator()(int value) const { return factor_ * value; }
  };

} // namespace mylib
