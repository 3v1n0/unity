set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set (CFLAGS ${CFLAGS} "-fPIC")
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")

add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION} -lm -lGL -lGLU")
link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Headers & Sources
#
set (PREVIEWS_SOURCES
     ApplicationPreview.cpp
     GenericPreview.cpp
     MusicPreview.cpp
     MoviePreview.cpp
     Preview.cpp
     PreviewContainer.cpp
     PreviewFactory.cpp
     PreviewNavigator.cpp
     )

add_library (previews-lib STATIC ${PREVIEWS_SOURCES})
add_dependencies (previews-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (previews-lib unity-shared)
# 
# Standalone variant
#
add_executable (previews Standalone.cpp)
add_dependencies (previews previews-lib)
target_link_libraries (previews previews-lib unity-shared)
