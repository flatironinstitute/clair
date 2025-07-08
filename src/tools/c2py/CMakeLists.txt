file(GLOB_RECURSE sources *.cpp)
add_executable(clair_c2py ${sources})

# Enable warnings
target_link_libraries(clair_c2py PRIVATE $<BUILD_INTERFACE:clair_warnings>)

# Configure target and compilation
target_compile_features(clair_c2py PRIVATE cxx_std_20)
set_property(TARGET clair_c2py PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(clair_c2py PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_definitions(clair_c2py PRIVATE CLANG_FORMAT="${CLANG_FORMAT}")

add_library(clang_includes INTERFACE)
target_include_directories(clang_includes SYSTEM INTERFACE ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIR})


# RENAME clang_plugin target...
#target_link_libraries(clair_c2py PRIVATE clang_plugin clu fmt::fmt)
target_link_libraries(clair_c2py PRIVATE clang_includes clu fmt::fmt LLVM clang-cpp)# LLVMSupport)

#target_link_libraries(clair_c2py PRIVATE clang_includes clu fmt::fmt clangTooling clangFrontend 
#      clangAST clangSerialization clangRewrite clangASTMatchers clangBasic )# LLVMSupport)



set_target_properties(clair_c2py PROPERTIES OUTPUT_NAME "clair-c2py")
install(TARGETS clair_c2py DESTINATION ${CMAKE_INSTALL_BINDIR})