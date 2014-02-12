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
set (STANDALONE_SOURCES
     StandaloneUnity.cpp
     )

add_executable (unity-standalone StandaloneUnity.cpp)
target_link_libraries (unity-standalone 
                       dash-lib
                       launcher-lib
                       panel-lib
                       unity-shared
                       unity-shared-bamf
                       unity-shared-compiz
                       unity-shared-standalone)
