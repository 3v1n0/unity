set(UNITY_SRC ../plugins/unityshell/src)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${PIC_FLAGS}
     )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} ${UNITY_STANDALONE_LADD})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (.. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

#
# Headers & Sources
#
set (PANEL_SOURCES
     PanelController.cpp
     PanelIndicatorEntryView.cpp
     PanelIndicatorsView.cpp
     PanelMenuView.cpp
     PanelTitlebarGrabAreaView.cpp
     PanelTray.cpp
     PanelView.cpp
     WindowButtons.cpp
     )

add_library (panel-lib STATIC ${PANEL_SOURCES})
add_dependencies (panel-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/panel_pch.hh panel-lib)

# 
# Standalone variant
#
add_executable (panel StandalonePanel.cpp)
target_link_libraries (panel panel-lib unity-shared unity-shared-standalone)
