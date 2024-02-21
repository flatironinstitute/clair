set(SRC_FILES ast_consumer.cpp matchers.cpp worker.cpp main.cpp)
add_executable(annotate_fnt ${SRC_FILES})

# Enable warnings
target_link_libraries(annotate_fnt PRIVATE $<BUILD_INTERFACE:clair_warnings>)

# Configure target and compilation
target_compile_features(annotate_fnt PRIVATE cxx_std_20)
set_property(TARGET annotate_fnt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(annotate_fnt PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(annotate_fnt PRIVATE clang_plugin clu fmt::fmt clangTooling clangFrontend clangAST clangSerialization clangRewrite clangASTMatchers clangBasic LLVMSupport)

# Install the tool
install(TARGETS annotate_fnt DESTINATION ${CMAKE_INSTALL_BINDIR})
