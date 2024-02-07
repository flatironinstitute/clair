#include <c2py/c2py.hpp>
#include <nda/nda.hpp>
#include <string>
#include <vector>
#include <functional>

// struct dummy {
//   std::function<double(nda::array_const_view<double, 1>)> f;
//   dummy(const std::function<double(nda::array_const_view<double, 1>)> &g) : f{g} {}
// };

struct dummy {
  std::function<double(nda::array<double, 1>)> f;
  //dummy(const std::function<double(nda::array<double, 1>)> &g) : f{g} {}
};

double ff(nda::array_const_view<double, 1> a) {
  int b = 0;
  return a(0) + b;
}

void modif1(nda::array_view<double, 1> a) { a *= 2; }

// Does not compile, not convertible from a Python view.
//void modif2(nda::array<double, 1> &a) { a *= 2; }