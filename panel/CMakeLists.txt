set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )
add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION} -lm -lGL -lGLU")
link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Headers & Sources
#
set (PANEL_SOURCES
     PanelController.cpp
     PanelIndicatorEntryView.cpp
     PanelIndicatorsView.cpp
     PanelMenuView.cpp
     PanelStyle.cpp
     PanelTitlebarGrabAreaView.cpp
     PanelTray.cpp
     PanelView.cpp
     WindowButtons.cpp
     WindowManager.cpp
     )

add_library (panel-lib STATIC ${PANEL_SOURCES})
add_dependencies (panel-lib unity-core-${UNITY_API_VERSION} unity-shared)

# 
# Standalone variant
#
add_executable (panel StandalonePanel.cpp)
add_dependencies (panel panel-lib)
target_link_libraries (panel panel-lib unity-shared)
