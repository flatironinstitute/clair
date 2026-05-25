# Protect against libc++ on Linux
try_compile(USE_LIBCPP ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libcpp_check.cpp)
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND USE_LIBCPP)
  message(FATAL_ERROR "Clair should be compiled without libc++ on Linux")
endif()

# ========= Additional Depdencies ==========

# LLVM / Clang
message(STATUS "-------- LLVM / Clang detection -------------")
include(clang_detection)

# LLVM / Flang
if("f2py" IN_LIST CLAIR_ENABLE_PROJECTS)
  message(STATUS "-------- LLVM / Flang detection (for clair-f2py) -------------")
  include(flang_detection)
endif()

# fmt
if(NOT TARGET fmt)
  message(STATUS "-------- Fetch & Configure FMT -------------")
  include(FetchContent)
  FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        main
  EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(fmt)
  set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
else()
  message(STATUS "-------- Reusing existing fmt target -------------")
endif()

# toml++
if(NOT TARGET tomlplusplus::tomlplusplus)
  message(STATUS "-------- Fetch & Configure toml++ -------------")
  include(FetchContent)
  FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(tomlplusplus)
else()
  message(STATUS "-------- Reusing existing tomlplusplus target -------------")
endif()

# ========= Dynamic Analyzer Checks ==========

option(ASAN "Compile library and executables with LLVM Address Sanitizer" OFF)
if(ASAN)
  if(NOT TARGET asan)
    find_package(sanitizer REQUIRED COMPONENTS asan)
  endif()
endif()

option(UBSAN "Compile library and executables with LLVM Undefined Behavior Sanitizer" OFF)
if(UBSAN)
  if(NOT TARGET ubsan)
    find_package(sanitizer REQUIRED COMPONENTS ubsan)
  endif()
endif()

option(MSAN "Compile library and executables with LLVM Memory Sanitizer" OFF)
if(MSAN)
  if(NOT TARGET msan)
    find_package(sanitizer REQUIRED COMPONENTS msan)
  endif()
endif()

option(TSAN "Compile library and executables with LLVM Thread Sanitizer" OFF)
if(TSAN)
  if(NOT TARGET tsan)
    find_package(sanitizer REQUIRED COMPONENTS tsan)
  endif()
endif()

# ========= Build all components ==========

add_subdirectory(clu)
add_subdirectory(tools)
