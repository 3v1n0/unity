add_subdirectory(pam)

# GSettings Schema
SET (UNITY_SCHEMAS "com.canonical.Unity.gschema.xml")
SET (UNITY_TEST_SCHEMAS "external.gschema.xml")

# Have an option to not install the schema into where GLib is
option (GSETTINGS_LOCALINSTALL "Install GSettings Schemas locally instead of to the GLib prefix" OFF)
if (GSETTINGS_LOCALINSTALL)
    SET (GSETTINGS_DIR "${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas/")
else (GSETTINGS_LOCALINSTALL)
    execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} glib-2.0 --variable prefix  OUTPUT_VARIABLE _glib_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET (GSETTINGS_DIR "${_glib_prefix}/share/glib-2.0/schemas/")
endif (GSETTINGS_LOCALINSTALL)

# Run the validator and error if it fails
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas  OUTPUT_VARIABLE _glib_comple_schemas OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${_glib_comple_schemas} --dry-run --schema-file=${CMAKE_CURRENT_SOURCE_DIR}/${UNITY_SCHEMAS} ERROR_VARIABLE _schemas_invalid OUTPUT_STRIP_TRAILING_WHITESPACE)

if (_schemas_invalid)
  message (SEND_ERROR "Schema validation error: ${_schemas_invalid}")
endif (_schemas_invalid)

# Actually install and recomple schemas
message (STATUS "GSettings schemas will be installed into ${GSETTINGS_DIR}")
install (FILES ${UNITY_SCHEMAS} DESTINATION ${GSETTINGS_DIR} OPTIONAL)
install (CODE "message (STATUS \"Compiling GSettings schemas\")")
install (CODE "execute_process (COMMAND ${_glib_comple_schemas} ${GSETTINGS_DIR})")

# Settings for testing
configure_file (${CMAKE_SOURCE_DIR}/data/${UNITY_SCHEMAS} ${CMAKE_BINARY_DIR}/settings/${UNITY_SCHEMAS})
configure_file (${CMAKE_SOURCE_DIR}/tests/data/${UNITY_TEST_SCHEMAS} ${CMAKE_BINARY_DIR}/settings/${UNITY_TEST_SCHEMAS})
execute_process (COMMAND ${_glib_comple_schemas} ${CMAKE_BINARY_DIR}/settings)

# Upstart
configure_file(unity7.conf.in ${CMAKE_CURRENT_BINARY_DIR}/unity7.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity7.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/sessions)

# Systemd
configure_file(unity7.service.in ${CMAKE_CURRENT_BINARY_DIR}/unity7.service @ONLY)
pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity7.service DESTINATION ${SYSTEMD_USER_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/unity7.override DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/systemd-session/upstart)
