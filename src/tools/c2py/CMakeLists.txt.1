file(GLOB_RECURSE sources *.cpp)
add_library(clair_c2py SHARED ${sources})
add_library(clair::clair_c2py ALIAS clair_c2py)

# Enable warnings
target_link_libraries(clair_c2py PRIVATE $<BUILD_INTERFACE:clair_warnings>)

# Configure target and compilation
target_compile_features(clair_c2py PUBLIC cxx_std_20)
set_target_properties(clair_c2py PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  PREFIX ""
)
target_include_directories(clair_c2py PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(clair_c2py PRIVATE CLANG_FORMAT="${CLANG_FORMAT}")
target_link_libraries(clair_c2py PRIVATE clang_plugin clu fmt::fmt)

install(TARGETS clair_c2py DESTINATION ${CMAKE_INSTALL_LIBDIR})

# ========= Dynamic Analyzer Checks ==========

if(ASAN)
  target_link_libraries(clair_c2py PRIVATE $<BUILD_INTERFACE:asan>)
endif()
if(UBSAN)
  target_link_libraries(clair_c2py PRIVATE $<BUILD_INTERFACE:ubsan>)
endif()

# ========== c2py_plugin target ===============

add_library(c2py_plugin INTERFACE)
add_dependencies(c2py_plugin clair_c2py)
target_compile_options(c2py_plugin
  INTERFACE
  $<BUILD_INTERFACE:-fplugin=${CMAKE_CURRENT_BINARY_DIR}/clair_c2py${CMAKE_SHARED_LIBRARY_SUFFIX}>
  $<INSTALL_INTERFACE:-fplugin=${CMAKE_INSTALL_FULL_LIBDIR}/clair_c2py${CMAKE_SHARED_LIBRARY_SUFFIX}>
)
target_compile_definitions(c2py_plugin INTERFACE C2PY_INCLUDED)
add_library(clair::c2py_plugin ALIAS c2py_plugin)

# ========== Install ===============

# Install the library in lib and export the c2py target
install(TARGETS c2py_plugin EXPORT clair-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})
