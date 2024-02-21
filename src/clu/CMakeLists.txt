file(GLOB_RECURSE sources *.cpp)
add_library(clu ${sources})
add_library(clair::clu ALIAS clu)

# Enable warnings
target_link_libraries(clu PRIVATE $<BUILD_INTERFACE:clair_warnings>)

# Configure target and compilation
target_compile_features(clu PRIVATE cxx_std_20)
set_target_properties(clu PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(clu PRIVATE $<BUILD_INTERFACE:clang_plugin>)
target_include_directories(clu PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(clu PRIVATE CLANG_RESOURCE_DIR="${LLVM_ROOT_DIR}/lib/clang/${LLVM_VERSION_MAJOR}")
