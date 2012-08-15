set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
  set (CFLAGS ${CFLAGS} "-fPIC")
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} ${UNITY_STANDALONE_LADD})

link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(previews)

#
# Headers & Sources
#
set (DASH_SOURCES
     AbstractPlacesGroup.cpp
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
     LensBar.cpp
     LensBarIcon.cpp
     LensView.cpp
     LensViewPrivate.cpp
     PlacesGroup.cpp
     ResultRenderer.cpp
     ResultRendererHorizontalTile.cpp
     ResultRendererTile.cpp
     ResultView.cpp
     ResultViewGrid.cpp
     )

add_library (dash-lib STATIC ${DASH_SOURCES})
add_dependencies (dash-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (dash-lib previews-lib)
# 
# Standalone variant
#
add_executable (dash StandaloneDash.cpp)
add_dependencies (dash dash-lib)
target_link_libraries (dash dash-lib unity-shared)
