set (PKGNAME "unity-${UNITY_API_VERSION}")

#
# unity.pc
#
set (PREFIX ${CMAKE_INSTALL_PREFIX})
set (DOLLAR "$") # You hear that? It's kittens being killed by the gods of cmake
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/unity.pc.cmake ${CMAKE_BINARY_DIR}/unity/${PKGNAME}.pc)
install (FILES ${CMAKE_BINARY_DIR}/unity/${PKGNAME}.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/)

#
# libunity
#
find_package(PkgConfig)
pkg_check_modules(DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-2.0 gthread-2.0 dee-1.0 gee-1.0 dbus-glib-1 clutter-1.0 clutk-0.3 gconf-2.0)

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

vala_precompile (VALA_C chrome-handler.vala
                        drag-controller.vala
                        drag-view.vala
                        entry.vala
                        icon-postprocessor.vala
                        prism-handler.vala
                        quicklist-rendering.vala
                        shell.vala
                        theme.vala
                        unity-appinfo-manager.vala
                        unity-cairo-canvas.vala
                        unity-expanding-bin.vala
                        unity-favorites.vala
                        unity-io.vala
                        unity-layered-bin.vala
                        unity-pixbuf-cache.vala
                        unity-place-activation.vala
                        unity-place-browser.vala
                        unity-place-renderer.vala
                        unity-place.vala
                        unity-stripe-texture.vala
                        webapp-fetcher.vala
                 PACKAGES gtk+-2.0 gio-2.0 glib-2.0 gobject-2.0 gee-1.0 dbus-glib-1 clutter-json-1.0 gconf-2.0 gio-unix-2.0
                 OPTIONS --thread
                 CUSTOM_VAPIS ../vapi/dee-1.0.vapi ../vapi/clutter-1.0.vapi ../vapi/config.vapi ../vapi/clutk-0.3.vapi ../vapi/cogl-1.0.vapi
                 GENERATE_VAPI unity
)

add_library (${PKGNAME} SHARED ${VALA_C})
install (TARGETS ${PKGNAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/)
install (FILES ${CMAKE_BINARY_DIR}/unity/unity.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PKGNAME}/unity/)
