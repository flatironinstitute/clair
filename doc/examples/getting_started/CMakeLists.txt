cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(getting-started-example LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# This example ships without generated bindings, so regenerate them with clair-c2py.
set(Update_Python_Bindings ON CACHE BOOL "Regenerate bindings via clair-c2py")

include(FetchContent)

# Fetch c2py; this also makes the c2py_add_module macro available.
FetchContent_Declare(c2py
  GIT_REPOSITORY https://github.com/flatironinstitute/c2py
  GIT_TAG unstable
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c2py)

# Build the getting_started Python module and (re)generate its bindings.
c2py_add_module(getting_started)
