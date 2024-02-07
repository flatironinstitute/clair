#include <c2py/c2py.hpp>

int g() { return 8;}

namespace N { 
  int f(int x) { return -x;}

  struct a_class{
    // ...
  };
  
  struct hidden{
    //...
  };
}

namespace c2py_module {
  auto match_names = "N::.*"; // only matches functions and names containing 
  auto reject_names = "N::hidden";
}


