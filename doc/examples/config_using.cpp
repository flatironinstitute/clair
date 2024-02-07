#include <c2py/c2py.hpp>

template<typename T> 
struct A {
  T x;
};

namespace c2py_module { 
    // ...
    namespace add {
      using Ai = A<int>;
      using Ad = A<double>;
   }
}


