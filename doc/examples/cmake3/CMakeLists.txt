cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(clairexample VERSION 3.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Locate Python and NumPy components
find_package(Python COMPONENTS Interpreter Development NumPy)

# Fetch the c2py library
include(FetchContent)
FetchContent_Declare(
  c2py
  GIT_REPOSITORY https://github.com/flatironinstitute/c2py
  GIT_TAG        unstable
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c2py)

# Build the Python C++ extension module
Python_add_library(my_module MODULE my_module.cpp)
target_link_libraries(my_module PRIVATE c2py::c2py)

# clair-c2py: optionally regenerate bindings
option(Update_Python_Bindings "Use clair python bindings generators" OFF)

if (Update_Python_Bindings)
  find_program(clair-c2py clair-c2py REQUIRED)

  add_custom_command(
    COMMAND ${clair-c2py} -p ${PROJECT_BINARY_DIR} my_module.cpp    # -p path/to/compile/commands
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}                   # Execute in source directory
    OUTPUT my_module.wrap.cxx                                       # Generates this file
    DEPENDS my_module.cpp                                           #
  )

  add_custom_target(wrap_gen DEPENDS my_module.wrap.cxx)
  add_dependencies(my_module wrap_gen)  # Ensure bindings are generated before compilation
endif()

