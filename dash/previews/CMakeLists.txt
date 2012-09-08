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

#
# Headers & Sources
#
set (PREVIEWS_SOURCES
     ActionButton.cpp
     ApplicationPreview.cpp
     GenericPreview.cpp
     MusicPreview.cpp
     MoviePreview.cpp
     Preview.cpp
     PreviewContainer.cpp
     PreviewInfoHintWidget.cpp
     PreviewNavigator.cpp
     PreviewRatingsWidget.cpp
     Track.cpp
     Tracks.cpp
     )

add_library (previews-lib STATIC ${PREVIEWS_SOURCES})
add_dependencies (previews-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (previews-lib unity-shared)

# 
# Application Standalone variant
#
add_executable (app_previews StandaloneApplicationPreview.cpp)
add_dependencies (app_previews previews-lib)
target_link_libraries (app_previews previews-lib unity-shared)

# 
# Music Standalone variant
#
add_executable (music_previews StandaloneMusicPreview.cpp)
add_dependencies (music_previews previews-lib)
target_link_libraries (music_previews previews-lib unity-shared)

# 
# Music Standalone variant
#
add_executable (movie_previews StandaloneMoviePreview.cpp)
add_dependencies (movie_previews previews-lib)
target_link_libraries (movie_previews previews-lib unity-shared)
