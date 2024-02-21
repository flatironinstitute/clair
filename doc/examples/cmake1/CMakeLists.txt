cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clairexample VERSION 3.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# Find Python, c2py and clair
find_package(Python COMPONENTS Interpreter Development NumPy)
find_package(c2py REQUIRED)
find_package(clair REQUIRED)

Python_add_library(my_module MODULE my_module.cpp)

# Link to low level c2py and add the -fplugin option using a cmake target. 
target_link_libraries(my_module PRIVATE c2py::c2py clair::c2py_plugin)
