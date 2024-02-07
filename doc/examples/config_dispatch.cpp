#include <c2py/c2py.hpp>

int f(auto x) { return 1;} // a generic (template) function

namespace c2py_module { 
    // ...
    namespace add {
      // function f in python dispatching the 2 instantiations.
      auto f = c2py::dispatch<::f<int>, ::f<double>>;
   }
}


