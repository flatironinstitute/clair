cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clairexample VERSION 3.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Python COMPONENTS Interpreter Development NumPy)
find_package(c2py REQUIRED)

# We can compile with or without the Clair plugin
option(GENERATE_PYTHON_BINDINGS "Use Clair python bindings generators" OFF)
if (GENERATE_PYTHON_BINDINGS)
  find_package(Clair REQUIRED)
endif()

# If GENERATE_PYTHON_BINDINGS, we compile my_module.cpp else the bindings my_module.wrap.cxx 
Python_add_library(my_module MODULE my_module.$<IF:$<BOOL:${GENERATE_PYTHON_BINDINGS}>,cpp,wrap.cxx>)

# Link to low level c2py. 
# Use the clair plugin if we want to generated the bindings 
target_link_libraries(my_module PRIVATE c2py::c2py $<$<BOOL:${GENERATE_PYTHON_BINDINGS}>:clair::c2py_plugin>)


