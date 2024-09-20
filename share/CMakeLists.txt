add_subdirectory(cmake)

if(NOT IS_SUBPROJECT
  AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr"
  AND NOT CMAKE_INSTALL_PREFIX STREQUAL "/usr/local"
)

  configure_file(clair.modulefile.in ${PROJECT_NAME}.modulefile @ONLY)
  configure_file(clairvars.sh.in ${PROJECT_NAME}vars.sh @ONLY)

  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/clair.modulefile
      ${CMAKE_CURRENT_BINARY_DIR}/clairvars.sh
    DESTINATION share/clair
  )
 
  message(STATUS "*********************************************************************************")
  message(STATUS "* Custom install Location. Use:                                                  ")
  message(STATUS "*                                                                                ")
  message(STATUS "*   source ${CMAKE_INSTALL_PREFIX}/share/clair/${PROJECT_NAME}vars.sh  ")
  message(STATUS "*                                                                                ")
  message(STATUS "* to set up the environment variables                                            ")
  if(DEFINED ENV{MODULEPATH})
    message(STATUS "*                                                                          ")
    message(STATUS "* Consider copying ${CMAKE_CURRENT_BINARY_DIR}/clair.modulefile  ")
    message(STATUS "*   into your environment module directories                               ")
  endif()
  message(STATUS "*********************************************************************************")

endif()
