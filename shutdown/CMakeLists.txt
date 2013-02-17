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
set (SHUTDOWN_SOURCES
     SessionButton.cpp
     SessionController.cpp
     SessionView.cpp
     )

add_library (shutdown-lib STATIC ${SHUTDOWN_SOURCES})
add_dependencies (shutdown-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/shutdown_pch.hh shutdown-lib)

#
# Standalone variant
#
add_executable (shutdown StandaloneSession.cpp)
target_link_libraries (shutdown shutdown-lib unity-shared unity-shared-standalone)
