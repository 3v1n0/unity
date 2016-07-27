#
# Some unity tools
#
install(FILES makebootchart.py PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${CMAKE_INSTALL_LIBDIR}/unity)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/unity.cmake ${CMAKE_BINARY_DIR}/bin/unity)
install(FILES ${CMAKE_BINARY_DIR}/bin/unity PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION ${CMAKE_INSTALL_BINDIR})

#
# unity-active-plugins-safety-check
#
pkg_check_modules(UAPSC_DEPS REQUIRED libcompizconfig)

set(CFLAGS
    ${UAPSC_DEPS_CFLAGS}
    ${UAPSC_DEPS_CFLAGS_OTHER}
    )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set(LIBS ${UAPSC_DEPS_LIBRARIES})

set(LIB_PATHS ${UAPSC_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

add_executable(unity-active-plugins-safety-check unity_active_plugins_safety_check.cpp)
target_link_libraries(unity-active-plugins-safety-check ${LIBS})
install(TARGETS unity-active-plugins-safety-check DESTINATION ${CMAKE_INSTALL_LIBDIR}/unity/)
