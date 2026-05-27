file(GLOB_RECURSE sources *.cpp)
add_library(flu ${sources})
add_library(clair::flu ALIAS flu)

# Must match Flang's own C++17 build to avoid header incompatibilities.
target_compile_features(flu PRIVATE cxx_std_17)
set_target_properties(flu PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(flu PRIVATE flang_llvm)
target_include_directories(flu PRIVATE ${PROJECT_SOURCE_DIR}/src)
