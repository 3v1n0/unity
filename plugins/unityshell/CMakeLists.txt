find_package (Compiz REQUIRED)

include (CompizPlugin)

set (UNITY_PLUGIN_DEPS "nux-0.9;libbamf3;dee-1.0;gio-2.0;gio-unix-2.0;dbusmenu-glib-0.4;x11;libstartup-notification-1.0;gthread-2.0;indicator3;atk;unity-misc;gconf-2.0;libutouch-geis;gtk+-3.0")

compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${PKGDATADIR}\"' -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/shared ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CFLAGS}"
               LIBRARIES "-lunity-shared-${UNITY_API_VERSION}"
               LIBDIRS "${CMAKE_BINARY_DIR}/shared"
               )
add_dependencies(unityshell unity-shared-${UNITY_API_VERSION})

install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)
