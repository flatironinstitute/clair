# Protect against libc++ on Linux
try_compile(USE_LIBCPP ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libcpp_check.cpp)
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND USE_LIBCPP)
  message(FATAL_ERROR "Clair should be compiled without libc++ on Linux")
endif()

add_subdirectory(clu)
add_subdirectory(plugins)
add_subdirectory(tools)
