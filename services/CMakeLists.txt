#
# Panel Service
#
find_package(PkgConfig)
pkg_check_modules(SERVICE_DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-2.0 gthread-2.0 indicator>=0.3.19 x11 gconf-2.0)

execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} indicator --variable indicatordir OUTPUT_VARIABLE _indicatordir OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} indicator --variable iconsdir OUTPUT_VARIABLE _iconsdir OUTPUT_STRIP_TRAILING_WHITESPACE)

find_program(GLIB_GENMARSHAL glib-genmarshal)
macro(add_glib_marshal outfiles name prefix otherinclude)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.c
    COMMAND ${GLIB_GENMARSHAL} ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.list --body --prefix=panel_marshal >> ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating marshallers")
  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.h
    COMMAND ${GLIB_GENMARSHAL} ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.list --header --prefix=panel_marshal >> ${CMAKE_CURRENT_SOURCE_DIR}/panel-marshal.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating marshallers")
#  list(APPEND ${outfiles} "${CMAKE_CURRENT_BINARY_DIR}/${name}.c")
endmacro(add_glib_marshal)

set(CFLAGS
    ${SERVICE_DEPS_CFLAGS}
    ${SERVICE_DEPS_CFLAGS_OTHER}
    ${MAINTAINER_CFLAGS}
    "-DGETTEXT_PACKAGE=\"unity\""
    "-DINDICATORDIR=\"${_indicatordir}\""
    "-DINDICATORICONDIR=\"${_iconsdir}\""
    "-Werror -Wall"
    )
add_definitions(${CFLAGS})

set(LIBS ${SERVICE_DEPS_LIBRARIES})
link_libraries(${LIBS})

set(LIB_PATHS ${SERVICE_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

add_executable(unity-panel-service
               panel-a11y.c
               panel-a11y.h
               panel-indicator-accessible.c
               panel-indicator-accessible.h
               panel-indicator-entry-accessible.c
               panel-indicator-entry-accessible.h
               panel-main.c
	       panel-marshal.c
	       panel-marshal.h
               panel-root-accessible.c
               panel-root-accessible.h
               panel-service.c
               panel-service.h
               panel-util-accessible.c
               panel-util-accessible.h
              )
install(TARGETS unity-panel-service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/unity/)

configure_file(com.canonical.Unity.Panel.Service.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
