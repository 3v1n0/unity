#
# Panel Service
#

set(UNITY_PANEL_SERVICE_DEPS
    atk
    atk-bridge-2.0
    gio-2.0>=2.30.0
    gobject-2.0
    gthread-2.0
    gtk+-3.0>=3.3
    indicator3-0.4>=12.10.2
    x11
    libido3-0.1>=13.0.0
    libupstart
)

pkg_check_modules(SERVICE_DEPS REQUIRED ${UNITY_PANEL_SERVICE_DEPS})

set(PANEL_SOURCES
    panel-a11y.c
    panel-a11y.h
    panel-indicator-accessible.c
    panel-indicator-accessible.h
    panel-indicator-entry-accessible.c
    panel-indicator-entry-accessible.h
    panel-main.c
    panel-root-accessible.c
    panel-root-accessible.h
    panel-service.c
    panel-service.h
    panel-util-accessible.c
    panel-util-accessible.h)

set(CFLAGS
    ${SERVICE_DEPS_CFLAGS}
    ${SERVICE_DEPS_CFLAGS_OTHER}
    "-Werror -Wall -Wno-error=deprecated-declarations"
    )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CFLAGS}")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(LIBS ${SERVICE_DEPS_LIBRARIES})

set(LIB_PATHS ${SERVICE_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

add_executable(unity-panel-service ${PANEL_SOURCES})
target_link_libraries(unity-panel-service ${LIBS})
install(TARGETS unity-panel-service DESTINATION ${UNITY_INSTALL_LIBDIR})

configure_file(unity-panel-service.conf.in ${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service.conf)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/sessions)

configure_file(unity-panel-service-lockscreen.conf.in ${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service-lockscreen.conf)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service-lockscreen.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/upstart/sessions)

##
##  Systemd Unit Files
##

# where to install
# Uncomment when we drop Vivid
# pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
set (SYSTEMD_USER_DIR "/usr/lib/systemd/user")
message (STATUS "${SYSTEMD_USER_DIR} is the systemd user unit file install dir")

configure_file (unity-panel-service.service.in "${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service.service")
configure_file (unity-panel-service-lockscreen.service.in "${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service-lockscreen.service")

install( FILES
         "${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service.service"
         "${CMAKE_CURRENT_BINARY_DIR}/unity-panel-service-lockscreen.service"
         "${CMAKE_CURRENT_SOURCE_DIR}/unity-screen-locked.target"
         DESTINATION "${SYSTEMD_USER_DIR}")

##
##  Upstart systemd override Job File
##

set (UPSTART_SYSTEMD_OVERRIDE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/upstart/systemd-session/upstart")
message (STATUS "${UPSTART_SYSTEMD_OVERRIDE_DIR} is the Upstart override Job File for systemd dir")

install (FILES 
		 unity-panel-service.override
		 unity-panel-service-lockscreen.override
         DESTINATION "${UPSTART_SYSTEMD_OVERRIDE_DIR}")
