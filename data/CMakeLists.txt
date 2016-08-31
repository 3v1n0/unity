add_subdirectory(pam)

# Upstart
configure_file(unity7.conf.in ${CMAKE_CURRENT_BINARY_DIR}/unity7.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity7.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/sessions)

# Systemd
configure_file(unity7.service.in ${CMAKE_CURRENT_BINARY_DIR}/unity7.service @ONLY)
pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity7.service DESTINATION ${SYSTEMD_USER_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/unity7.override DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/systemd-session/upstart)
