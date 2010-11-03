cmake_minimum_required(VERSION 2.8)

subdirs (unity)

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
