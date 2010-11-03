cmake_minimum_required(VERSION 2.8)

subdirs (unity services tests/unit)

#
# Base bits
#
set (PROJECT_NAME "unity")
set (UNITY_MAJOR 3)
set (UNITY_MINOR 0)
set (UNITY_MICRO 1)
set (UNITY_VERSION "${UNITY_MAJOR}.${UNITY_MINOR}.${UNITY_MICRO}")
set (UNITY_API_VERSION "3.0")

#
# config.h
#
set (PREFIXDIR "${CMAKE_INSTALL_PREFIX}")
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/unity/${UNITY_MAJOR}")
set (GETTEXT_PACKAGE "unity")
set (LOCALE_DIR "${DATADIR}/locale")
set (VERSION "${UNITY_VERSION}")

configure_file (${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

#
# Data
#
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
install (FILES ${_datafiles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/unity/${UNITY_MAJOR}/)

#
# i18n
#
find_package (Gettext REQUIRED)

add_custom_command (OUTPUT ${CMAKE_SOURCE_DIR}/po/unity.pot
		COMMAND xgettext -c --files-from ${CMAKE_SOURCE_DIR}/po/POTFILES.in --keyword=_ -o ${CMAKE_SOURCE_DIR}/po/unity.pot --copyright-holder="Canonical Ltd" --msgid-bugs-address="ayatana-dev@lists.launchpad.net" --no-wrap --no-location
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)

if (${GETTEXT_FOUND} STREQUAL "TRUE")
        set (HAVE_GETTEXT true)
        file (GLOB _translations ${CMAKE_SOURCE_DIR}/po/*.po)
        GETTEXT_CREATE_TRANSLATIONS (${CMAKE_SOURCE_DIR}/po/${PROJECT_NAME}.pot
                ALL ${_translations})
endif (${GETTEXT_FOUND} STREQUAL "TRUE")

#
# src (Compiz Plugin)
#
set (UNITY_PLUGIN_DEPS "nux-0.9 libbamf dbus-glib-1 dee-1.0 gio-2.0")

find_package (Compiz REQUIRED)
include (CompizPlugin)
compiz_plugin (unity 
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${CMAKE_INSTALL_PREFIX}/share/unity/3\"' -I${CMAKE_BINARY_DIR}")

#
# GSettings Schema
#
SET (UNITY_SCHEMAS "com.canonical.Unity.gschema.xml")

# Have an option to not install the schema into where GLib is
option (GSETTINGS_LOCALINSTALL "Install GSettings Schemas locally instead of to the GLib prefix" OFF)
if (GSETTINGS_LOCALINSTALL)
    SET (GSETTINGS_DIR "${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas/")
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
