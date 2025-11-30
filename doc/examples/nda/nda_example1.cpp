#include <c2py/c2py.hpp>
#include <nda/nda.hpp>

double my_sum(nda::array_const_view<double, 1> a) { return sum(a); }

nda::array<int, 1> make_array(int n) { return {n, n + 1}; }

#include "nda_example1.wrap.cxx"
