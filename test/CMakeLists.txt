# Tell c2py it is run within clair.
# c2py is a subproject but in this specific case, we want to build its tests 
set(IS_CLAIR_BUILD ON) 

FetchContent_Declare(
  c2py
  GIT_REPOSITORY  https://github.com/flatironinstitute/c2py
  GIT_TAG         unstable
  EXCLUDE_FROM_ALL # exclude from all so that c2py is NOT installed.
)
FetchContent_MakeAvailable(c2py)

file(COPY ${PROJECT_SOURCE_DIR}/.clang-format DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# We add the tests back to the ALL to have them run
add_custom_target(c2py_make_all_tests ALL DEPENDS ${c2py_all_full_tests} ${c2py_all_low_level_tests})
