set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
  set (CFLAGS ${CFLAGS} "-fPIC")
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} ${UNITY_STANDALONE_LADD})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Headers & Sources
#
set (UNITY_SHARED_SOURCES
     AbstractSeparator.cpp
     Animator.cpp
     BGHash.cpp
     CoverArt.cpp
     BackgroundEffectHelper.cpp
     DashStyle.cpp
     DebugDBusInterface.cpp
     DefaultThumbnailProvider.cpp
     FontSettings.cpp
     IMTextEntry.cpp
     IconLoader.cpp
     IconRenderer.cpp
     IconTexture.cpp
     IconTextureSource.cpp
     Introspectable.cpp
     IntrospectableWrappers.cpp
     JSONParser.cpp
     LineSeparator.cpp
     OverlayRenderer.cpp
     PanelStyle.cpp
     PlacesVScrollBar.cpp
     PreviewStyle.cpp
     RatingsButton.cpp
     SearchBar.cpp
     SearchBarSpinner.cpp
     StaticCairoText.cpp
     TextureCache.cpp
     TextInput.cpp
     TextureThumbnailProvider.cpp
     ThumbnailGenerator.cpp
     Timer.cpp
     UBusServer.cpp
     UBusWrapper.cpp
     UScreen.cpp
     UnitySettings.cpp
     UnityWindowStyle.cpp
     UnityWindowView.cpp
     UserThumbnailProvider.cpp
     WindowManager.cpp
     XPathQueryPart.cpp
)

if(UNITY_ENABLE_X_ORG_SUPPORT)
  set (UNITY_SHARED_SOURCES
    XKeyboardUtil.cpp
    XWindowManager.cpp
    ${UNITY_SHARED_SOURCES}
    )
else()
  set (UNITY_SHARED_SOURCES
    FakeKeyboardUtil.cpp
    ${UNITY_SHARED_SOURCES}
    )
endif()

add_library (unity-shared STATIC ${UNITY_SHARED_SOURCES})
target_link_libraries (unity-shared ${LIBS})
add_dependencies (unity-shared unity-core-${UNITY_API_VERSION})

#
# We also need to build compiz specific parts and standalone variants of those parts
#

if(UNITY_ENABLE_X_ORG_SUPPORT)
  # compiz
  set (UNITY_SHARED_COMPIZ_SOURCES
    PluginAdapter.cpp
    )
  add_library (unity-shared-compiz STATIC ${UNITY_SHARED_COMPIZ_SOURCES})
  target_link_libraries (unity-shared-compiz ${LIBS})

  add_dependencies (unity-shared-compiz unity-shared)
endif()

# standalone
set (UNITY_SHARED_STANDALONE_SOURCES
     StandaloneWindowManager.cpp
     )
add_library (unity-shared-standalone STATIC ${UNITY_SHARED_STANDALONE_SOURCES})
target_link_libraries (unity-shared-standalone ${LIBS})
add_dependencies (unity-shared-standalone unity-shared)


