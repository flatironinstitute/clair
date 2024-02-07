# --- Python ---
find_package(Python COMPONENTS Interpreter Development NumPy)
#set(Python_all_flags " -I${Python_INCLUDE_DIRS} -I${Python_NumPy_INCLUDE_DIRS} -L${Python_LIBRARY_DIRS} -lpython${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
#configure_file(clair_c2py_flags.py.in clair_c2py_flags @ONLY)

set(PYTHON_LIB_DEST ${PYTHON_LIB_DEST_ROOT}/clair)

# Install
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/compiler.py  ${CMAKE_CURRENT_SOURCE_DIR}/c2py_magic.py  ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
        DESTINATION ${PYTHON_LIB_DEST})

#install(FILES ${CMAKE_CURRENT_BINARY_DIR}/clair_c2py_flags DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
