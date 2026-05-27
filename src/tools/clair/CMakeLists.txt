if("c2py" IN_LIST CLAIR_ENABLE_PROJECTS)
  add_tool(clair-c2py DIR c2py LLVM_LIBS clang_llvm
    "codegen/*.cpp"
    "ir/types.cpp"
    "ir/types_clang.cpp"
    "module_info.cpp"
  )
endif()

if("f2py" IN_LIST CLAIR_ENABLE_PROJECTS)
  add_tool(clair-f2py DIR f2py LLVM_LIBS flang_llvm
    "codegen/*.cpp"
    "ir/types.cpp"
    "ir/types_flang.cpp"
    "module_info.cpp"
  )
  target_link_libraries(clair-f2py PRIVATE clair::flu)
  # Flang itself is built with C++17; compile f2py/ sources with C++17 to match.
  # Moreover, compilation breaks for C++XX > C++17.
  # ir/types_flang.cpp also includes Flang headers and needs the same treatment.
  file(GLOB F2PY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/f2py/*.cpp")
  set_source_files_properties(
    ${F2PY_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/ir/types_flang.cpp"
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:Clang>:-std=c++17>"
  )
endif()
