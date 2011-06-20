find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${PKGDATADIR}\"' -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/core ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CFLAGS}"
               LIBRARIES "-lunity-core-${UNITY_API_VERSION}"
               LIBDIRS "${CMAKE_BINARY_DIR}/core"
               )
add_dependencies(unityshell unity-core-${UNITY_API_VERSION})

install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)
