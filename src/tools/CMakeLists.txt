function(add_tool TOOL_NAME)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "DIR;LLVM_LIBS" "")

  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_DIR})
    message(FATAL_ERROR "Tool directory '${ARG_DIR}' does not exist.")
  endif()

  file(GLOB_RECURSE TOOL_SOURCES "${ARG_DIR}/*.cpp" ${ARG_UNPARSED_ARGUMENTS})
  add_executable(${TOOL_NAME} ${TOOL_SOURCES})

  target_link_libraries(${TOOL_NAME} PRIVATE $<BUILD_INTERFACE:clair_warnings>)
  target_compile_features(${TOOL_NAME} PRIVATE cxx_std_20)
  target_include_directories(${TOOL_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(${TOOL_NAME} PRIVATE clu fmt::fmt tomlplusplus::tomlplusplus ${ARG_LLVM_LIBS})
  set_target_properties(${TOOL_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

  install(TARGETS ${TOOL_NAME} DESTINATION bin)
endfunction()

add_tool(annotate-fnt DIR annotate-fnt LLVM_LIBS clang_llvm)
add_subdirectory(clair)
