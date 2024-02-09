set(SRC_FILES ast_consumer.cpp matchers.cpp worker.cpp main.cpp)
add_executable(annotate_fnt ${SRC_FILES} )

target_link_libraries(annotate_fnt clang_plugin clu fmt::fmt clangTooling)

target_include_directories(annotate_fnt PRIVATE ${CMAKE_SOURCE_DIR}/c++ ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET annotate_fnt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(annotate_fnt PRIVATE cxx_std_20)

install(TARGETS annotate_fnt DESTINATION bin)
