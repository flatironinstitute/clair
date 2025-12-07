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

include(${c2py_SOURCE_DIR}/share/cmake/clair_c2py_generate_bindings.cmake)  
#include(clair_c2py_generate_bindings.cmake)  

# Macro to build a Python C++ extension module with c2py bindings
macro(add_clair_module module_name)
  Python_add_library(${module_name} MODULE ${module_name}.cpp)
  target_link_libraries(${module_name} PRIVATE c2py::c2py)
  clair_c2py_generate_bindings(${module_name})
endmacro()

# Build the Python C++ extension module
add_subdirectory(gallery)
add_subdirectory(getting_started)
add_subdirectory(code_annotations)

add_clair_module(fun1)
add_clair_module(function_template)
add_clair_module(my_enums)

# kept for error message generation in the doc
# add_clair_module(fail)

