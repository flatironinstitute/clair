set(SRC_FILES ast_consumer.cpp matchers.cpp worker.cpp main.cpp)
add_executable(parent2bracket ${SRC_FILES})

# Enable warnings
target_link_libraries(parent2bracket PRIVATE $<BUILD_INTERFACE:clair_warnings>)

# Configure target and compilation
target_compile_features(parent2bracket PRIVATE cxx_std_20)
set_property(TARGET parent2bracket PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(parent2bracket PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parent2bracket PRIVATE clang_plugin clu fmt::fmt clangTooling clangFrontend clangAST clangSerialization clangRewrite clangASTMatchers clangBasic LLVMSupport)

# Install the tool
install(TARGETS parent2bracket DESTINATION bin)
