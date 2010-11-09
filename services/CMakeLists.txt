#
# Panel Service
#
find_package(PkgConfig)
pkg_check_modules(SERVICE_DEPS REQUIRED gobject-2.0 gio-2.0 gtk+-2.0 gthread-2.0 indicator)

set(CFLAGS ${SERVICE_DEPS_CFLAGS} ${SERVICE_DEPS_CFLAGS_OTHER} "-DGETTEXT_PACKAGE=\"unity\"")
add_definitions(${CFLAGS})

set(LIBS ${SERVICE_DEPS_LIBRARIES})
link_libraries(${LIBS})

set(LIB_PATHS ${SERVICE_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

add_executable(unity-panel-service
               panel-main.c
               panel-service.c
               panel-service.h 
              )
install(TARGETS unity-panel-service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/unity/)

configure_file(com.canonical.Unity.Panel.Service.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.Unity.Panel.Service.service DESTINATION ${CMAKE_INSTALL_PREFIX}/share/dbus-1/services)
