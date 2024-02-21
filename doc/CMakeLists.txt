# Generate the sphinx config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in ${CMAKE_CURRENT_BINARY_DIR}/conf.py @ONLY)

# ---------------------------------
# Top Sphinx target
# ---------------------------------
find_package(Sphinx REQUIRED)

MESSAGE(STATUS "SPHINXBUILD_EXECUTABLE = ${SPHINXBUILD_EXECUTABLE}")

# Sphinx has internal caching, always run it
add_custom_target(clair_docs_sphinx ALL)
add_custom_command(
  TARGET clair_docs_sphinx
  COMMAND PYTHONPATH=${PROJECT_BINARY_DIR}/python:${h5_BINARY_DIR}/python:$ENV{PYTHONPATH} ${SPHINXBUILD_EXECUTABLE} -c . -j8 -b html ${CMAKE_CURRENT_SOURCE_DIR} html
)

# ---------------------------------
# Install
# ---------------------------------
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/ COMPONENT documentation DESTINATION share/doc/clair
  FILES_MATCHING
  REGEX "\\.(html|pdf|png|gif|jpg|svg|ico|js|xsl|css|py|txt|inv|bib|ttf|woff2|eot)$"
  PATTERN "_*"
)
