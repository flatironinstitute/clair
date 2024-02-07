add_library(clu misc.cpp concept.cpp fullqualifiedname.cpp compile.cpp 
                doc_string.cpp cmd_line_arg.cpp)

target_include_directories(clu PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(clu PRIVATE $<BUILD_INTERFACE:clang_plugin>)

target_compile_features(clu PRIVATE cxx_std_20)
set_target_properties(clu PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(clu PRIVATE -DCLANG_RESOURCE_DIR="${LLVM_ROOT_DIR}/lib/clang/${LLVM_VERSION_MAJOR}")

