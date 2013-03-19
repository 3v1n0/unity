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
set (SHORTCUTS_SOURCES
     AbstractShortcutHint.h
     AbstractShortcutModeller.h
     BaseWindowRaiserImp.cpp
     CompizShortcutModeller.cpp
     MockShortcutHint.h
     ShortcutController.cpp
     ShortcutController.h
     ShortcutHint.cpp
     ShortcutHintPrivate.cpp
     ShortcutModel.cpp
     ShortcutModel.h
     ShortcutView.cpp
     ShortcutView.h
     )

add_library (shortcuts-lib STATIC ${SHORTCUTS_SOURCES})
add_dependencies (shortcuts-lib unity-core-${UNITY_API_VERSION} unity-shared)
add_pch(pch/shortcuts_pch.hh shortcuts-lib)

# 
# Standalone variant
#
add_executable (shortcuts StandaloneShortcuts.cpp)
target_link_libraries (shortcuts shortcuts-lib unity-shared unity-shared-standalone)
