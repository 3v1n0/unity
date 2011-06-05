project (unity)
cmake_minimum_required(VERSION 2.8)


include (cmake/Documentation.cmake)

subdirs (services tests tools doc)

#
# Base bits
#
set (PROJECT_NAME "unity")
set (UNITY_MAJOR 3)
set (UNITY_MINOR 8)
set (UNITY_MICRO 12)
set (UNITY_VERSION "${UNITY_MAJOR}.${UNITY_MINOR}.${UNITY_MICRO}")
set (UNITY_API_VERSION "3.0")

#
# Niceties
#
set (ARCHIVE_NAME unity-${UNITY_VERSION})
add_custom_target (dist
    COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

add_custom_target (distcheck
    COMMAND cd ${CMAKE_BINARY_DIR}
    && rm -rf ${ARCHIVE_NAME}
    && tar xf ${ARCHIVE_NAME}.tar.bz2
    && mkdir ${ARCHIVE_NAME}/build
    && cd ${ARCHIVE_NAME}/build
    && cmake -DCMAKE_INSTALL_PREFIX=../install -DGSETTINGS_LOCALINSTALL=ON .. -DCMAKE_MODULE_PATH=/usr/share/cmake
    && make -j8
    && make -j8 install
    && make check
    )
add_dependencies(distcheck dist)

#
# config.h
#
set (PREFIXDIR "${CMAKE_INSTALL_PREFIX}")
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/unity/${UNITY_MAJOR}")
set (GETTEXT_PACKAGE "unity")
set (LOCALE_DIR "${DATADIR}/locale")
set (VERSION "${UNITY_VERSION}")
set (BUILDDIR "${CMAKE_BINARY_DIR}")

configure_file (${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

#
# Data
#
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
install (FILES ${_datafiles} DESTINATION ${PKGDATADIR})

install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)

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
# Enable or disable boot logging
# 
option (BOOT_LOGGER "Enable startup performance logging" OFF)
if (BOOT_LOGGER)
    SET (BOOT_LOGGER_FLAG "-DENABLE_LOGGER")
endif (BOOT_LOGGER)

SET (MAINTAINER_CFLAGS "-Werror -Wall -Wcast-align -Wno-uninitialized -Wempty-body -Wformat-security -Winit-self")
option (DISABLE_MAINTAINER_CFLAGS "Disable maintainer CFlags" OFF)
if (DISABLE_MAINTAINER_CFLAGS)
  SET (MAINTAINER_CFLAGS "")
endif (DISABLE_MAINTAINER_CFLAGS)

#
# src (Compiz Plugin)
#
set (UNITY_PLUGIN_DEPS "nux-0.9;libbamf;dee-1.0;gio-2.0;gio-unix-2.0;dbusmenu-glib-0.4;x11;libstartup-notification-1.0;gthread-2.0;indicator;atk;unity-misc;gconf-2.0;libutouch-geis")

find_package (Compiz REQUIRED)
include (CompizPlugin)
compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${CMAKE_INSTALL_PREFIX}/share/unity/3\"' -I${CMAKE_BINARY_DIR} ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CFLAGS}"
               )

#
# UTouch things
#

compiz_add_plugins_in_folder ("${CMAKE_SOURCE_DIR}/utouch")

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

# For testing
configure_file (${CMAKE_SOURCE_DIR}/${UNITY_SCHEMAS} ${CMAKE_BINARY_DIR}/settings/${UNITY_SCHEMAS})
execute_process (COMMAND ${_glib_comple_schemas} ${CMAKE_BINARY_DIR}/settings)

#
# docs
#

# check if doxygen is even installed
find_package(Doxygen)
if (DOXYGEN_FOUND STREQUAL "NO")
  message("Doxygen not found. Documentation will not be built")
endif (DOXYGEN_FOUND STREQUAL "NO")

if (DOXYGEN_FOUND STREQUAL "YES")
  set(TOP_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

  # prepare doxygen configuration file
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # add doxygen as target
  add_custom_target(doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # cleanup $build/api-doc on "make clean"
  set_property(DIRECTORY APPEND PROPERTY
         ADDITIONAL_MAKE_CLEAN_FILES api-doc)

  # install HTML API documentation and manual pages
  set(DOC_PATH "share/doc/${CPACK_PACKAGE_NAME}-${VERSION}")

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/api-doc/html
           DESTINATION ${DOC_PATH})
endif (DOXYGEN_FOUND STREQUAL "YES")
