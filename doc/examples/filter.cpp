#include <c2py/c2py.hpp>

int g() { return 8; }

namespace N {
  int f(int x) { return -x; }

  struct a_class {
    // ...
  };

  struct hidden {
    //...
  };
} // namespace N

namespace c2py_module {
  auto match_names  = "N::.*";     // match functions and names in namespace N
  auto reject_names = "N::hidden"; // reject the specific name "hidden" in namespace N
} // namespace c2py_module
