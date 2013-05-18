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

add_subdirectory(previews)

#
# Headers & Sources
#
set (DASH_SOURCES
     ApplicationStarterImp.cpp
     CoverflowResultView.cpp
     DashController.cpp
     DashView.cpp
     DashViewPrivate.cpp
     FilterAllButton.cpp
     FilterBar.cpp
     FilterBasicButton.cpp
     FilterExpanderLabel.cpp
     FilterFactory.cpp
     FilterGenreButton.cpp
     FilterGenreWidget.cpp
     FilterMultiRangeButton.cpp
     FilterMultiRangeWidget.cpp
     FilterRatingsButton.cpp
     FilterRatingsWidget.cpp
     ScopeBar.cpp
     ScopeBarIcon.cpp
     ScopeView.cpp
     PlacesGroup.cpp
     PreviewStateMachine.cpp
     ResultRenderer.cpp
     ResultRendererHorizontalTile.cpp
     ResultRendererTile.cpp
     ResultView.cpp
     ResultViewGrid.cpp
     )

add_library (dash-lib STATIC ${DASH_SOURCES})
add_dependencies (dash-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (dash-lib previews-lib)
add_pch(pch/dash_pch.hh dash-lib)


# 
# Standalone variant
#
add_executable (dash StandaloneDash.cpp)
target_link_libraries (dash dash-lib unity-shared unity-shared-standalone)
