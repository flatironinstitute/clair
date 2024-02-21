#===============================================================================
# LOAD LLVM CONFIGURATION
#===============================================================================
set(LLVM_CONFIG llvm-config CACHE STRING "Path to llvm-config")

execute_process(COMMAND ${LLVM_CONFIG} --prefix OUTPUT_VARIABLE LLVM_ROOT_DIR_DEFAULT OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set this to a valid LLVM installation dir
set(LLVM_ROOT_DIR "${LLVM_ROOT_DIR_DEFAULT}") #CACHE PATH "LLVM installation directory")

# Add the location of ClangConfig.cmake to CMake search paths (so that
# find_package can locate it)
list(PREPEND CMAKE_PREFIX_PATH "${LLVM_ROOT_DIR}/lib/cmake/clang/") 
list(PREPEND CMAKE_PREFIX_PATH "${LLVM_ROOT_DIR}/lib/cmake/llvm/") 
# We must find the proper LLVM config

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

set(CLANG_EXECUTABLE "${CLANG_INSTALL_PREFIX}/bin/clang++")

MESSAGE(STATUS "LLVM_ROOT_DIR_DEFAULT : ${LLVM_ROOT_DIR_DEFAULT}")
MESSAGE(STATUS "LLVM_ROOT_DIR : ${LLVM_ROOT_DIR}")
MESSAGE(STATUS "LLVM_VERSION_MAJOR : ${LLVM_VERSION_MAJOR}")
MESSAGE(STATUS "LLVM_INCLUDE_DIR : ${LLVM_INCLUDE_DIR}")
MESSAGE(STATUS "LLVM_LIBRARY_DIR : ${LLVM_LIBRARY_DIR}")
MESSAGE(STATUS "CLANG_INCLUDE_DIR : ${CLANG_INCLUDE_DIR}")
MESSAGE(STATUS "CLANG_EXECUTABLE : ${CLANG_EXECUTABLE}")

#===============================================================================
#  Find clang-format
#===============================================================================
find_program (CLANG_FORMAT NAMES clang-format clang-format-${LLVM_VERSION_MAJOR} PATHS ${CLANG_INSTALL_PREFIX})
MESSAGE(STATUS "CLANG_FORMAT : ${CLANG_FORMAT}")

#===============================================================================
# Create an Interface target for compiler plugins
#===============================================================================
add_library(clang_plugin INTERFACE)
target_include_directories(clang_plugin SYSTEM INTERFACE ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIR})

# Allow undefined symbols in shared objects on Darwin (this is the default behaviour on Linux)
target_link_libraries(clang_plugin INTERFACE "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")

if(NOT LLVM_ENABLE_RTTI)
 target_compile_options(clang_plugin INTERFACE -fno-rtti)
endif()
