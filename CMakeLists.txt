subdirs (unity)

#
# config.h
#
set (PREFIXDIR "${CMAKE_INSTALL_PREFIX}")
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/unity/3")
set (GETTEXT_PACKAGE "unity")
set (LOCALE_DIR "${DATADIR}/locale")
set (VERSION "0.3.1")

configure_file (${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_SOURCE_DIR}/config.h)
