function(add_tool TOOL_NAME)
  if(NOT IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_NAME})
    message(FATAL_ERROR "Tool directory '${TOOL_NAME}' does not exist.")
  endif()

  file(GLOB_RECURSE TOOL_SOURCES "${TOOL_NAME}/*.cpp" ${ARGN})

  add_executable(${TOOL_NAME} ${TOOL_SOURCES})
  
  # Enable warnings
  target_link_libraries(${TOOL_NAME} PRIVATE $<BUILD_INTERFACE:clair_warnings>)
  
  # Configure annoteate tool target
  target_compile_features(${TOOL_NAME} PRIVATE cxx_std_20)
  target_include_directories(${TOOL_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(${TOOL_NAME} PRIVATE clu fmt::fmt tomlplusplus::tomlplusplus clang_llvm)
  set_target_properties(${TOOL_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  
  # Install the tool
  install(TARGETS ${TOOL_NAME} DESTINATION bin)
endfunction()

add_tool(annotate-fnt)
add_tool(clair-c2py "codegen/*.cpp" "ir/*.cpp")
