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

# FetchContent_MakeAvailable(c2py) above also provides the c2py_add_module macro.

# Build the Python C++ extension modules
add_subdirectory(gallery)
add_subdirectory(getting_started)
add_subdirectory(code_annotations)
#add_subdirectory(nda)

c2py_add_module(fun1)
c2py_add_module(function_template)
c2py_add_module(my_enums)
add_subdirectory(classes)

# kept for error message generation in the doc
# c2py_add_module(fail)

