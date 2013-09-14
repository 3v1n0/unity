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
set (PREVIEWS_SOURCES
     ActionButton.cpp
     ActionLink.cpp
     ApplicationPreview.cpp
     GenericPreview.cpp
     MusicPreview.cpp
     MoviePreview.cpp
     PaymentPreview.cpp
     Preview.cpp
     PreviewContainer.cpp
     PreviewInfoHintWidget.cpp
     PreviewNavigator.cpp
     PreviewRatingsWidget.cpp
     SocialPreview.cpp
     SocialPreviewContent.cpp
     SocialPreviewComments.cpp
     TabIterator.cpp
     Track.cpp
     Tracks.cpp
     MusicPaymentPreview.cpp
     ErrorPreview.cpp
     )

add_library (previews-lib STATIC ${PREVIEWS_SOURCES})
add_dependencies (previews-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (previews-lib unity-shared)
add_pch(pch/previews_pch.hh previews-lib)

#
# Application Standalone variant
#
add_executable (app_previews StandaloneApplicationPreview.cpp)
target_link_libraries (app_previews previews-lib unity-shared)

#
# Music Standalone variant
#
add_executable (music_previews StandaloneMusicPreview.cpp)
target_link_libraries (music_previews previews-lib unity-shared)

#
# Social Standalone variant
#
add_executable (social_previews StandaloneSocialPreview.cpp)
target_link_libraries (social_previews previews-lib unity-shared)

#
# Movie Standalone variant
#
add_executable (movie_previews StandaloneMoviePreview.cpp)
target_link_libraries (movie_previews previews-lib unity-shared)

#
# Payment Standalone variant
#
add_executable (payment_previews StandaloneMusicPaymentPreview.cpp)
add_dependencies (payment_previews previews-lib)
target_link_libraries (payment_previews previews-lib unity-shared)

#
# Error Standalone variant
#
add_executable (error_previews StandaloneErrorPreview.cpp)
add_dependencies (error_previews previews-lib)
target_link_libraries (error_previews previews-lib unity-shared)
