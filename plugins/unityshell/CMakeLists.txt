find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${PKGDATADIR}\"' -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR} ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CFLAGS}"
               LIBRARIES "-lunity-core-${UNITY_API_VERSION}"
               LIBDIRS "${CMAKE_BINARY_DIR}/UnityCore"
               )
add_dependencies(unityshell unity-core-${UNITY_API_VERSION})

#
# Data
#
install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)
install (FILES resources/dash-widgets.json DESTINATION ${CMAKE_INSTALL_PREFIX}/share/unity/themes)
 
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
install (FILES ${_datafiles} DESTINATION ${PKGDATADIR})
