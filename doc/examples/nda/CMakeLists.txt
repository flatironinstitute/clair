find_package(nda)
if (NDA_FOUND) # the redondancy is for the rst inclusion in doc

Python_add_library(nda_example1 MODULE nda_example1.cpp)
target_link_libraries(nda_example1 PRIVATE c2py::c2py nda::nda_c nda::nda_py)
clair_c2py_generate_bindings(nda_example1)

else()  

# Fetch NDA library
set(Build_Tests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    nda
    GIT_REPOSITORY https://github.com/TRIQS/nda.git
    GIT_TAG        FixC2pyConv
)
FetchContent_MakeAvailable(nda)

# A macro to show how build the module with nda
macro(add_clair_module_with_nda module_name)
  Python_add_library(${module_name} MODULE ${module_name}.cpp)
  target_link_libraries(${module_name} PRIVATE c2py::c2py nda::nda_c) #  nda::nda_py)
  clair_c2py_generate_bindings(${module_name})
endmacro()

add_clair_module_with_nda(nda_example1)

# Because it is fetched.
target_include_directories(nda_example1 PRIVATE ${nda_SOURCE_DIR}/python)

endif()