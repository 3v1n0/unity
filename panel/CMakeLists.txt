set(UNITY_SRC ../plugins/unityshell/src)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${PIC_FLAGS}
     )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set (LIBS ${CACHED_UNITY_DEPS_LDFLAGS} ${UNITY_STANDALONE_LADD})

include_directories (.. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

#
# Headers & Sources
#
set (PANEL_SOURCES
     PanelController.cpp
     PanelIndicatorEntryView.cpp
     PanelIndicatorEntryDropdownView.cpp
     PanelIndicatorsView.cpp
     PanelMenuView.cpp
     PanelTitlebarGrabAreaView.cpp
     PanelTray.cpp
     PanelView.cpp
     )

add_library (panel-lib STATIC ${PANEL_SOURCES})
add_dependencies (panel-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/panel_pch.hh panel-lib)

# 
# Standalone variant
#
add_executable (panel StandalonePanel.cpp)
target_link_libraries (panel panel-lib unity-shared unity-shared-standalone)
