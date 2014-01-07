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
set (LOCKSCREEN_SOURCES
     BackgroundSettingsGnome.cpp
     CofView.cpp
     LockScreenController.cpp
     LockScreenShield.cpp
     UserAuthenticatorPam.cpp
     )

add_library (lockscreen-lib STATIC ${LOCKSCREEN_SOURCES})
add_dependencies (lockscreen-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/lockscreen_pch.hh lockscreen-lib)

#
# Standalone variant
#
add_executable (lockscreen StandaloneLockScreen.cpp)
target_link_libraries (lockscreen lockscreen-lib unity-shared unity-shared-standalone)
