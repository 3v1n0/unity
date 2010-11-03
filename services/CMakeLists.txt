#
# Panel Service
#
find_package(PkgConfig)
pkg_check_modules(DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-2.0 gthread-2.0 indicator dee-1.0 gee-1.0 dbus-glib-1)

set(CFLAGS
	${DEPS_CFLAGS} ${DEPS_CFLAGS_OTHER}
  "-I/usr/include/gee-1.0" "-DGETTEXT_PACKAGE=\"unity\""
)
add_definitions(${CFLAGS})

set(LIBS
	${DEPS_LIBRARIES}
  "-lgee"
)
link_libraries(${LIBS})

set(LIB_PATHS
	${DEPS_LIBRARY_DIRS}
)
link_directories(${LIB_PATHS})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/vala)
include(ValaPrecompile)
include(ValaVersion)

find_package (Vala)

vala_precompile (VALA_C panel-service.vala
                 PACKAGES gtk+-2.0 gio-2.0 glib-2.0 gobject-2.0 gee-1.0 dbus-glib-1
                 OPTIONS --thread
                 CUSTOM_VAPIS vapi/indicator.vapi vapi/dee-1.0.vapi vapi/gdk-extra.vapi
)

add_executable(unity-panel-service ${VALA_C})
install(TARGETS unity-panel-service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/unity/)

configure_file(com.canonical.Unity.Panel.Service.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
