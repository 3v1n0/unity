subdirs (unity)

#
# Base bits
#
set (UNITY_MAJOR 3)
set (UNITY_MINOR 0)
set (UNITY_MICRO 1)
set (UNITY_VERSION "${UNITY_MAJOR}.${UNITY_MINOR}.${UNITY_MICRO}")


#
# config.h
#
set (PREFIXDIR "${CMAKE_INSTALL_PREFIX}")
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/unity/${UNITY_MAJOR}")
set (GETTEXT_PACKAGE "unity")
set (LOCALE_DIR "${DATADIR}/locale")
set (VERSION "${UNITY_VERSION}")

configure_file (${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_SOURCE_DIR}/config.h)

#
# Data
#
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
install (FILES ${_datafiles} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/unity/${UNITY_MAJOR}/)
