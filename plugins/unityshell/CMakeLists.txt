find_package (Compiz REQUIRED)

include (CompizPlugin)

set (COMPIZ_PLUGIN_INSTALL_TYPE "package")

# Guard against Compiz altering global state.
# https://bugs.launchpad.net/compiz/+bug/1096807
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(revert_compiz TRUE)
endif()

compiz_plugin (unityshell
               PKGDEPS ${UNITY_PLUGIN_DEPS}
               PLUGINDEPS composite opengl compiztoolbox scale
               CFLAGSADD "-DINSTALLPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DPKGDATADIR='\"${PKGDATADIR}\"' -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR} ${BOOT_LOGGER_FLAG} -DGETTEXT_PACKAGE='\"unity\"' ${MAINTAINER_CXXFLAGS} -I${CMAKE_SOURCE_DIR}/dash/ -I${CMAKE_SOURCE_DIR}/launcher/ -I${CMAKE_SOURCE_DIR}/lockscreen/ -I${CMAKE_SOURCE_DIR}/hud/ -I${CMAKE_SOURCE_DIR}/panel/ -I${CMAKE_SOURCE_DIR}/shortcuts/ -I${CMAKE_SOURCE_DIR}/shutdown/ -I${CMAKE_SOURCE_DIR}/unity-shared/"
               LIBDIRS "${CMAKE_BINARY_DIR}/UnityCore"
               )

if(revert_compiz)
  set (CMAKE_BUILD_TYPE "" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif()

add_dependencies(unityshell unity-core-${UNITY_API_VERSION} dash-lib launcher-lib lockscreen-lib switcher-lib hud-lib panel-lib shortcuts-lib shutdown-lib unity-shared unity-shared-compiz)
target_link_libraries(unityshell unity-core-${UNITY_API_VERSION} launcher-lib lockscreen-lib dash-lib switcher-lib hud-lib panel-lib shortcuts-lib shutdown-lib unity-shared unity-shared-bamf unity-shared-compiz)
set_target_properties(unityshell
                      PROPERTIES INSTALL_RPATH "${CACHED_UNITY_PRIVATE_DEPS_LIBRARY_DIRS}"
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

#
# Data
#
install (FILES plugin-unityshell.png DESTINATION ${DATADIR}/ccsm/icons/hicolor/64x64/apps)
