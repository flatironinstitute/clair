set(SRC_FILES
  plugin.cpp
  matchers.cpp
  ast_consumer.cpp
  data.cpp
  worker.cpp
  codegen/doc.cpp
  codegen/fnt.cpp
  codegen/module.cpp
  codegen/classes.cpp
)

# separate for later reuse in the jit branch
add_library(clair_c2py SHARED ${SRC_FILES})
target_compile_definitions(clair_c2py PRIVATE CLANG_FORMAT="${CLANG_FORMAT}")
target_include_directories(clair_c2py PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(clair_c2py PRIVATE ${PROJECT_NAME}_warnings clang_plugin clu fmt::fmt)
#target_link_libraries(clair_c2py PRIVATE -L${LLVM_ROOT_DIR_DEFAULT}/lib -lclangFormat)#
target_compile_features(clair_c2py PRIVATE cxx_std_20)
set_target_properties(clair_c2py PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(clair_c2py PROPERTIES PREFIX "")

install(TARGETS clair_c2py DESTINATION lib)

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
target_compile_options(c2py_plugin INTERFACE $<BUILD_INTERFACE:-fplugin=${CMAKE_CURRENT_BINARY_DIR}/clair_c2py${CMAKE_SHARED_LIBRARY_SUFFIX}>)
target_compile_options(c2py_plugin INTERFACE $<INSTALL_INTERFACE:-fplugin=${CMAKE_INSTALL_PREFIX}/lib/clair_c2py${CMAKE_SHARED_LIBRARY_SUFFIX}>)
#target_compile_definitions(c2py_plugin INTERFACE C2PY_INCLUDED)
target_compile_options(c2py_plugin INTERFACE -DC2PY_INCLUDED  -Wno-unused-const-variable -Wno-unused-variable)
#target_link_libraries(c2py_plugin INTERFACE c2py::c2py)

add_library(clair::c2py_plugin ALIAS c2py_plugin)

# ========== Install ===============

# Install the library in lib and export the c2py target
install(TARGETS c2py_plugin EXPORT clair-targets DESTINATION lib)
#install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION include FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")

# Install the exported targets
install(EXPORT clair-targets NAMESPACE clair:: DESTINATION lib/cmake/clair)

# =========  magic, clair.py info module ? ===========
add_subdirectory(python)




