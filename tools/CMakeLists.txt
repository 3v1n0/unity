#
# Some unity tools
#
install(FILES makebootchart.py migrate_favorites.py PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/unity/)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/unity.cmake ${CMAKE_BINARY_DIR}/bin/unity)
install(FILES ${CMAKE_BINARY_DIR}/bin/unity PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin)

#
# unity-preferences
#
find_package(PkgConfig)
pkg_check_modules(PREFS_DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-3.0 gthread-2.0)

set(CFLAGS
    ${PREFS_DEPS_CFLAGS}
    ${PREFS_DEPS_CFLAGS_OTHER}
    ${MAINTAINER_CFLAGS}
    "-DGETTEXT_PACKAGE=\"unity\""
    "-Werror -Wall"
    )
add_definitions(${CFLAGS})

include_directories (. .. ${CMAKE_BINARY_DIR})

set(LIBS ${PREFS_DEPS_LIBRARIES})
link_libraries(${LIBS})

set(LIB_PATHS ${PREFS_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

add_executable(unity-preferences unity-preferences.c)
install(TARGETS unity-preferences DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
install (FILES unity-preferences.desktop DESTINATION ${DATADIR}/applications)
