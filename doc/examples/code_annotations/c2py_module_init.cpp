// c2py_module_init.cpp

#include "c2py/c2py.hpp"
#include <iostream>

// This function should be called when the module is initialized (imported) in Python.
C2PY_MODULE_INIT void init() { std::cout << "c2py/clair rocks!" << std::endl; }
