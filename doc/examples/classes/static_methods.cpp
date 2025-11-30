// Static methods
#include <c2py/c2py.hpp>

namespace mylib {

  class math_utils {
    public:
    // Static methods
    static int add(int a, int b) { return a + b; }
    static int multiply(int a, int b) { return a * b; }

    // Static factory method
    static math_utils create() { return math_utils(); }
  };

} // namespace mylib
