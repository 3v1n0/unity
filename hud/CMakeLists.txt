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
set (HUD_SOURCES
     HudAbstractView.cpp
     HudButton.cpp
     HudController.cpp
     HudIcon.cpp
     HudIconTextureSource.cpp
     HudPrivate.cpp
     HudView.cpp
    ) 

add_library (hud-lib STATIC ${HUD_SOURCES})
add_dependencies (hud-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/hud_pch.hh hud-lib)

# 
# Standalone variant
#
add_executable (hud StandaloneHud.cpp)
target_link_libraries (hud hud-lib unity-shared unity-shared-standalone)
