cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(VERSION 3.20)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()
include(FetchContent)

project(clair VERSION 0.1.0 LANGUAGES C CXX)
get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)
if (IS_SUBPROJECT)
 MESSAGE(FATAL_ERROR "Clair can not be a subproject. Not tested.")
endif()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 MESSAGE(FATAL_ERROR "This must be compiled with Clang")
endif()

message( STATUS "-------- clair version and git hash detection -------------")
find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
 WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
 OUTPUT_VARIABLE   PROJECT_GIT_HASH
 OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "${PROJECT_NAME} version : ${PROJECT_VERSION}")
message(STATUS "${PROJECT_NAME} Git hash: ${PROJECT_GIT_HASH}")

# Check CMAKE_INSTALL_PREFIX : must be provided and absolute
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR (NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX}))
 message(FATAL_ERROR "CMAKE_INSTALL_PREFIX must be specified and must be an absolute path.\n There is no default.\n Current value : ${CMAKE_INSTALL_PREFIX}\n.  e.g. ... -DCMAKE_INSTALL_PREFIX=$HOME/clair_install")
endif()
message(STATUS "Installation directory will be ${CMAKE_INSTALL_PREFIX}")

#--------------------------------------------------------
#   Options
#--------------------------------------------------------

# Default to Release build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Type of build" FORCE)
endif()
message(STATUS "-------- BUILD-TYPE: ${CMAKE_BUILD_TYPE} -------------")

# Global compiler options
option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" OFF)


# Testing
option(Build_Tests "Build tests" ON)
if (Build_Tests)
 enable_testing()
endif()

# Sanitizers
option(ASAN  "Compile library and executables with LLVM Address Sanitizer" OFF)
option(UBSAN "Compile library and executables with LLVM Undefined Behavior Sanitizer" OFF)

#--------------------------------------------------------
#   clang/ llvm detection
#--------------------------------------------------------
# add ./cmake to the path for module
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/share/cmake/Modules)

message(STATUS "-------- clang detection -------------")
include(clang_detection)

#--------------------------------------------------------
#   FMT
#--------------------------------------------------------

FetchContent_Declare(
 fmt
 GIT_REPOSITORY https://github.com/fmtlib/fmt.git
 GIT_TAG        master 
 EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)

#--------------------------------------------------------
#   Python
#--------------------------------------------------------

message(STATUS "-------- Python detection -------------")
find_package(Python COMPONENTS Interpreter Development NumPy)

# set some vars for clairvars and some clair.py
# Compute the Python lib destination
if(BUILD_DEBIAN_PACKAGE)
set(PYTHON_LIB_DEST_DIR dist-packages)
else()
set(PYTHON_LIB_DEST_DIR site-packages)
endif()
set(PYTHON_LIB_DEST_ROOT lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/${PYTHON_LIB_DEST_DIR})

#MESSAGE("PYTHON_LIB_DEST_ROOT = ${PYTHON_LIB_DEST_ROOT}")
#MESSAGE("Python_STDLIB = ${Python_STDLIB}")
#MESSAGE("Python_SITELIB = ${Python_SITELIB}")
#MESSAGE("Python_STDARCH = ${Python_STDARCH}")
#MESSAGE("Python_LIBRARY_DIRS = ${Python_LIBRARY_DIRS}")
#MESSAGE("Python_SABI_LIBRARY_DIRS = ${Python_SABI_LIBRARY_DIRS}")

#--------------------------------------------------------
# Sources
#--------------------------------------------------------

add_subdirectory(src)
add_subdirectory(share/cmake) # Exported Clair-Config

#--------------------------------------------------------
# tests (only the h5 one, the c2py tests are in c2py)
#--------------------------------------------------------
if (Build_Tests)
 add_subdirectory(test)
endif()

#--------------------------------------------------------
#   Global Compilation Settings
#--------------------------------------------------------

# Provide additional debugging information for Debug builds
add_compile_options($<$<CONFIG:Debug>:-ggdb3>)

set(${PROJECT_NAME}_BINARY_DIR ${PROJECT_BINARY_DIR} CACHE STRING "Binary directory of the ${PROJECT_NAME} Project")

# Export the list of compile-commands into compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#--------------------------------------------------------
# Create an Interface target for compiler warnings
#--------------------------------------------------------

add_library(${PROJECT_NAME}_warnings INTERFACE)
target_compile_options(${PROJECT_NAME}_warnings
  INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-sign-compare
    $<$<CXX_COMPILER_ID:GNU>:-Wshadow=local>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>
    $<$<CXX_COMPILER_ID:Clang>:-Wshadow>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-gcc-compat>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wshadow>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wno-gcc-compat>
)

# ---------------------------------
# Resolve Clang Linktime Problems
# CMake will adjust any linker flags from '-L path_to/mylib.so' to -lmylib
# if the proper mylib.so is automatically found by the linker, i.e.
# the directory comes first in LIBRARY_PATH.
# The clang linker however ignores LIBRARY_PATH.
# We thus explicitly add the content of LIBRARY_PATH to the LDFLAGS
# FIXME For future cmake versions we should populate the
# ---------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND DEFINED ENV{LIBRARY_PATH})
  string(REPLACE ":" ";" LINK_DIRS $ENV{LIBRARY_PATH})
  foreach(dir ${LINK_DIRS})
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -L${dir}")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -L${dir}")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -L${dir}")
  endforeach()
endif()

#--------------------------------------------------------
#   Documentation
# ------------------------------------------------------
if(Build_Documentation)
 add_subdirectory(doc)
endif()

#--------------------------------------------------------
#   Sanitizers
#--------------------------------------------------------

if(ASAN)
  if(NOT TARGET asan)
    find_package(sanitizer REQUIRED "asan")
  endif()
endif()
if(UBSAN)
  if(NOT TARGET ubsan)
    find_package(sanitizer REQUIRED "ubsan")
  endif()
endif()

#--------------------------------------------------------
#   Remind the user how to set up his/her variables
#--------------------------------------------------------

if(NOT IS_SUBPROJECT
  AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr"
  AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr/local"
)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     set(LD_LIBRARY_PATH_PREFIX "DYLD")
  else() 
     set(LD_LIBRARY_PATH_PREFIX "LD")
  endif()

  # Configure and install the file to source to setup the environment variables
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clairvars.sh.in ${CMAKE_CURRENT_BINARY_DIR}/clairvars.sh @ONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clair.modulefile.in ${CMAKE_CURRENT_BINARY_DIR}/clair.modulefile @ONLY)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/clairvars.sh DESTINATION share/clair)
  message(STATUS "***************************************************************")
  message(STATUS "* Use :                                                        ")
  message(STATUS "*   source ${CMAKE_INSTALL_PREFIX}/share/clair/clairvars.sh  ")
  message(STATUS "*   to set up the environment variables                        ")
  if(DEFINED ENV{MODULEPATH})
    message(STATUS "*                                                                ")
    message(STATUS "* Consider copying ${CMAKE_CURRENT_BINARY_DIR}/clair.modulefile ")
    message(STATUS "*   into your environment module directories                     ")
  endif()
  message(STATUS "***************************************************************")
endif()

## uninstall target
#if(NOT TARGET uninstall)
    #configure_file(
        #"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        #"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        #IMMEDIATE @ONLY)

    #add_custom_target(uninstall
        #COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
#endif()

#--------------------------------------------------------
# Packaging
#--------------------------------------------------------
option(BUILD_DEBIAN_PACKAGE "Build a deb package" OFF)
if(BUILD_DEBIAN_PACKAGE)
  if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    message(FATAL_ERROR "CMAKE_INSTALL_PREFIX must be /usr for packaging")
  endif()
  set(CPACK_PACKAGE_NAME clair)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_CONTACT "https://github.com/TRIQS/clair")
  execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "python")
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
  include(CPack)
endif()
