#include <c2py/c2py.hpp>

enum class E1 { a, b, c };

enum E2 { A, B, C };

E1 f1(E1 x) { return x; }
E2 f2(E2 x) { return x; }

#include "my_enums.wrap.cxx"
