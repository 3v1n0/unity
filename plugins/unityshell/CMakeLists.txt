find_package (Compiz REQUIRED)

include (CompizPlugin)

set (COMPIZ_PLUGIN_INSTALL_TYPE "package")

compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl compiztoolbox scale
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${PKGDATADIR}\"' -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR} ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CXXFLAGS} -I${CMAKE_SOURCE_DIR}/dash/ -I${CMAKE_SOURCE_DIR}/launcher/ -I${CMAKE_SOURCE_DIR}/hud/ -I${CMAKE_SOURCE_DIR}/panel/ -I${CMAKE_SOURCE_DIR}/shortcuts/ -I${CMAKE_SOURCE_DIR}/unity-shared/"
               LIBDIRS "${CMAKE_BINARY_DIR}/UnityCore"
               )
add_dependencies(unityshell unity-core-${UNITY_API_VERSION} dash-lib launcher-lib switcher-lib hud-lib panel-lib shortcuts-lib unity-shared unity-shared-compiz)
target_link_libraries(unityshell unity-core-${UNITY_API_VERSION} launcher-lib dash-lib switcher-lib hud-lib panel-lib shortcuts-lib unity-shared unity-shared-compiz)
set_target_properties(unityshell 
                      PROPERTIES INSTALL_RPATH "${CACHED_UNITY_PRIVATE_DEPS_LIBRARY_DIRS}"
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

#
# Data
#
install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)
install (FILES resources/dash-widgets.json DESTINATION ${CMAKE_INSTALL_PREFIX}/share/unity/themes)
 
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
install (FILES ${_datafiles} DESTINATION ${PKGDATADIR})
