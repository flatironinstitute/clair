configure_file(clair-config.cmake.in clair-config.cmake @ONLY)
configure_file(clair-config-version.cmake.in clair-config-version.cmake @ONLY)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/clair-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/clair-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clair
)

#install(EXPORT clair-targets NAMESPACE clair:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clair)
