// fun2.cpp

#include <c2py/c2py.hpp>
#include <vector>
#include <numeric>

double sum(std::vector<double> const &v) { return std::accumulate(begin(v), end(v), 0.0); }
