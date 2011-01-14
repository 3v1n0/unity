set (PKGNAME unity)

#
# unity.pc
#
set (PREFIX ${CMAKE_INSTALL_PREFIX})
set (DOLLAR "$") # You hear that? It's kittens being killed by the gods of cmake
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/unity.pc.cmake ${CMAKE_BINARY_DIR}/libunity/${PKGNAME}.pc)
install (FILES ${CMAKE_BINARY_DIR}/libunity/${PKGNAME}.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/)
install (FILES ${CMAKE_BINARY_DIR}/libunity/${PKGNAME}.vapi DESTINATION ${CMAKE_INSTALL_PREFIX}/share/vala/vapi)

#
# libunity
#
find_package(PkgConfig)
pkg_check_modules(DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-2.0 gthread-2.0 dee-1.0 gee-1.0 gconf-2.0)

# Bump this one when a binary-incompatible change is introduced
set (libunity_SOVERSION ${UNITY_MAJOR})

# Bump this one when the API is extended in a binary-compatible way
set (libunity_API_VERSION 0)

# Bump this one when changes do not extend the API
set (libunity_PATCH_VERSION 0)

set (libunity_VERSION ${libunity_SOVERSION}.${libunity_API_VERSION}.${libunity_PATCH_VERSION})

set(CFLAGS
	${DEPS_CFLAGS} ${DEPS_CFLAGS_OTHER}
  "-I/usr/include/gee-1.0" -DGETTEXT_PACKAGE=\"unity\" -I${CMAKE_BINARY_DIR}
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

vala_precompile (VALA_C unity-place-activation.vala
                        unity-place-browser.vala
                        unity-place.vala
                        perf-logger.vala
                 PACKAGES gtk+-2.0 gio-2.0 glib-2.0 gobject-2.0 gee-1.0 gio-unix-2.0
                 OPTIONS --thread
                 CUSTOM_VAPIS ../vapi/dee-1.0.vapi ../vapi/config.vapi
                 GENERATE_VAPI unity
)

add_library (${PKGNAME} SHARED ${VALA_C} ubus-server.c)
set_target_properties (${PKGNAME} PROPERTIES
                       VERSION ${libunity_VERSION}
                       SOVERSION ${libunity_SOVERSION}
                       )


install (TARGETS ${PKGNAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install (FILES ${CMAKE_BINARY_DIR}/libunity/unity.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PKGNAME}/unity/)
