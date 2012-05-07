set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )
add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION} -lm -lGL -lGLU")
link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Headers & Sources
#
set (UNITY_SHARED_SOURCES
     Animator.cpp
     BGHash.cpp
     BackgroundEffectHelper.cpp
     DashSettings.cpp
     DashStyle.cpp
     FontSettings.cpp
     IMTextEntry.cpp
     IconLoader.cpp
     IconRenderer.cpp
     IconTexture.cpp
     IconTextureSource.cpp
     Introspectable.cpp
     IntrospectableWrappers.cpp
     JSONParser.cpp
     OverlayRenderer.cpp
     PanelStyle.cpp
     PluginAdapter.cpp
     SearchBar.cpp
     SearchBarSpinner.cpp
     StaticCairoText.cpp
     TextureCache.cpp
     Timer.cpp
     UBusWrapper.cpp
     UScreen.cpp
     WindowManager.cpp
     ubus-server.cpp
     )

add_library (unity-shared STATIC ${UNITY_SHARED_SOURCES})
add_dependencies (unity-shared unity-core-${UNITY_API_VERSION})

