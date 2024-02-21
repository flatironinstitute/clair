# Protect against libc++ on Linux
try_compile(USE_LIBCPP ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libcpp_check.cpp)
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND USE_LIBCPP)
  message(FATAL_ERROR "Clair should be compiled without libc++ on Linux")
endif()

# ========= Additional Depdencies ==========

# LLVM / Clang
message(STATUS "-------- LLVM / Clang detection -------------")
include(clang_detection)

# fmt
message(STATUS "-------- Fetch & Configure FMT -------------")
include(FetchContent)
FetchContent_Declare(
 fmt
 GIT_REPOSITORY https://github.com/fmtlib/fmt.git
 GIT_TAG        master 
 EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)
set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)

# ========= Dynamic Analyzer Checks ==========

option(ASAN OFF "Compile library and executables with LLVM Address Sanitizer")
if(ASAN)
  if(NOT TARGET asan)
    find_package(sanitizer REQUIRED COMPONENTS asan)
  endif()
endif()

option(UBSAN OFF "Compile library and executables with LLVM Undefined Behavior Sanitizer")
if(UBSAN)
  if(NOT TARGET ubsan)
    find_package(sanitizer REQUIRED COMPONENTS ubsan)
  endif()
endif()

option(MSAN OFF "Compile library and executables with LLVM Memory Sanitizer")
if(MSAN)
  if(NOT TARGET msan)
    find_package(sanitizer REQUIRED COMPONENTS msan)
  endif()
endif()

option(TSAN OFF "Compile library and executables with LLVM Thread Sanitizer")
if(TSAN)
  if(NOT TARGET tsan)
    find_package(sanitizer REQUIRED COMPONENTS tsan)
  endif()
endif()

# ========= Build all components ==========

add_subdirectory(clu)
add_subdirectory(plugins)
add_subdirectory(tools)
