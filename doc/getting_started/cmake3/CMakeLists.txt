cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clairexample VERSION 3.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

# Locate Python and NumPy components
find_package(Python COMPONENTS Interpreter Development NumPy)

# Fetch the c2py library
FetchContent_Declare(
  c2py
  GIT_REPOSITORY https://github.com/flatironinstitute/c2py
  GIT_TAG        unstable
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c2py)

# Build the Python C++ extension module
#
Python_add_library(my_module MODULE my_module.cpp)
target_link_libraries(my_module PRIVATE c2py::c2py)

# -----------------------------------------------
# Optionally regenerate bindings with clair-c2py
# -----------------------------------------------
include(${c2py_SOURCE_DIR}/share/cmake/clair_c2py_generate_bindings.cmake)  

option(Update_Python_Bindings "Use clair python bindings generators" ON)

if (Update_Python_Bindings)
  clair_c2py_generate_bindings(my_module)
endif()

