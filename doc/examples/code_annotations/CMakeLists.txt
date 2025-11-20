cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(code-annotations VERSION 3.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

# Locate Python and NumPy components
find_package(Python COMPONENTS Interpreter Development NumPy)

# Fetch the c2py library
FetchContent_Declare(
  c2py
  GIT_REPOSITORY https://github.com/flatironinstitute/c2py
  GIT_TAG        unstable
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(c2py)

# clair-c2py: optionally regenerate bindings
option(Update_Python_Bindings "Use clair python bindings generators" OFF)
if(Update_Python_Bindings)
  find_program(clair-c2py clair-c2py REQUIRED)
endif()

# Add an interface target for all Python modules
add_library(${PROJECT_NAME}_python_modules INTERFACE)

# Build and install any python modules
set(module_sources c2py_ignore.cpp c2py_rename.cpp c2py_module_init.cpp c2py_wrap_as_method.cpp)
foreach(module_src ${module_sources})
  get_filename_component(module_name ${module_src} NAME_WE)
  get_filename_component(module_dir ${module_src} DIRECTORY)

  Python_add_library(${module_name} MODULE ${module_src})
  add_library(${PROJECT_NAME}::${module_name} ALIAS ${module_name})
  target_link_libraries(${module_name} PRIVATE c2py::c2py)
  add_dependencies(${PROJECT_NAME}_python_modules ${module_name})

  if(Update_Python_Bindings)
    set(wrap_cxx "${CMAKE_CURRENT_SOURCE_DIR}/${module_name}.wrap.cxx")
    set(depfile "${CMAKE_CURRENT_BINARY_DIR}/${module_name}.cpp.d")

    add_custom_command(
      OUTPUT ${wrap_cxx} ${depfile}
      COMMAND ${clair-c2py} -p ${PROJECT_BINARY_DIR} --generate-depfile ${depfile} ${module_src}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      DEPFILE ${depfile}
    )

    add_custom_target(wrap_gen_${module_name} DEPENDS ${wrap_cxx})
    add_dependencies(${module_name} wrap_gen_${module_name})
  endif()
endforeach()
