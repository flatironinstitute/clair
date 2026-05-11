#=========================
# Locate Clang and LLVM
#=========================

execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
  OUTPUT_VARIABLE CLANG_RESOURCE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CLANG_RESOURCE_DIR "${CLANG_RESOURCE_DIR}" CACHE PATH "Clang resource directory")

find_package(Clang REQUIRED CONFIG HINTS ${LLVM_ROOT} $ENV{LLVM_ROOT} ${Clang_ROOT} $ENV{Clang_ROOT} ${CLANG_RESOURCE_DIR}/../../..)
if(LLVM_VERSION VERSION_LESS 20.0.0)
  message(FATAL_ERROR "LLVM version ${LLVM_VERSION} is not supported. Please use LLVM 20.0.0 or later. Use LLVM_ROOT to specify a different LLVM installation.")
endif()

message(STATUS "LLVM_VERSION : ${LLVM_VERSION}")
message(STATUS "LLVM_INSTALL_PREFIX : ${LLVM_INSTALL_PREFIX}")
message(STATUS "LLVM_INCLUDE_DIRS : ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_LIBRARY_DIRS : ${LLVM_LIBRARY_DIRS}")
message(STATUS "CLANG_INSTALL_PREFIX : ${CLANG_INSTALL_PREFIX}")
message(STATUS "CLANG_RESOURCE_DIR : ${CLANG_RESOURCE_DIR}")
message(STATUS "CLANG_INCLUDE_DIRS : ${CLANG_INCLUDE_DIRS}")

# On OS X, we check SDKROOT. If not present, the tool will 
# set this environement variable at start (for its own process only) before running.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  execute_process(
      COMMAND xcrun --show-sdk-path
      OUTPUT_VARIABLE SDKROOT
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(SDKROOT "${SDKROOT}" CACHE PATH "Clang SDKROOT")
  message(STATUS "Detected SDKROOT: ${SDKROOT}")
endif()

#===============================================================
# Create an Interface target for the Clang and LLVM Libraries
#===============================================================
add_library(clang_llvm INTERFACE)
target_link_libraries(clang_llvm INTERFACE clang-cpp $<$<PLATFORM_ID:Linux>:LLVM>)
target_include_directories(clang_llvm SYSTEM INTERFACE ${CLANG_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # Use LLVM provided libcxx
  target_link_directories(clang_llvm INTERFACE ${LLVM_INSTALL_PREFIX}/lib/c++)

  # Allow undefined symbols in shared objects on Darwin (this is the default behaviour on Linux)
  target_link_libraries(clang_llvm INTERFACE "-undefined dynamic_lookup")
endif()

if(NOT LLVM_ENABLE_RTTI)
 target_compile_options(clang_llvm INTERFACE -fno-rtti)
endif()
