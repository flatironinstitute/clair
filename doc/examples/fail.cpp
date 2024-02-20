#include <c2py/c2py.hpp>

double *make_raw_pointer(long size) { return new double[size]; }

void f(FILE *x) {}
