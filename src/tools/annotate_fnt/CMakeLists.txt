set(SRC_FILES ast_consumer.cpp matchers.cpp worker.cpp main.cpp)
add_executable(annotate_fnt ${SRC_FILES})

# Enable warnings
target_link_libraries(annotate_fnt PRIVATE $<BUILD_INTERFACE:clair_warnings>)

#find_package(zstd REQUIRED CONFIG)

# Configure target and compilation
target_compile_features(annotate_fnt PRIVATE cxx_std_20)
set_property(TARGET annotate_fnt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(annotate_fnt PRIVATE ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

#target_link_libraries(annotate_fnt PRIVATE clang_plugin clu fmt::fmt clang-cpp)

target_link_libraries(annotate_fnt PRIVATE clang_plugin clu fmt::fmt clangTooling clangFrontend clangAST clangSerialization clangRewrite clangASTMatchers clangBasic )# LLVMSupport)

# On some machine (Ubuntu), and only with lld linker, we need to link LLVM too.
# Not by default, and in some other machine, if we link against LLVM while we do not need to
# it causes issue with init of some global variables at run time.
#if (LINK_AGAINST_LLVM) 
 #target_link_libraries(annotate_fnt PRIVATE LLVM )
#endif()

## Add a specific flags on ld (not lld). 
#include(CheckCXXCompilerFlag)
#MESSAGE("CMAKE_LINKER = ${CMAKE_LINKER}")

#execute_process( COMMAND ${CMAKE_LINKER} --version OUTPUT_VARIABLE LINKER_VERSION_OUTPUT OUTPUT_STRIP_TRAILING_WHITESPACE)
#string(FIND "${LINKER_VERSION_OUTPUT}" "GNU ld" is_gnu_ld)
#if (NOT is_gnu_ld EQUAL -1)
   #message(STATUS "Linker is GNU ld.bfd — adding --copy-dt-needed-entries")
   ##target_link_options(annotate_fnt PRIVATE "-Wl,--copy-dt-needed-entries")
#endif()


# Install the tool
install(TARGETS annotate_fnt DESTINATION bin)
