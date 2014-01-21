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
set (UNITY_SHARED_SOURCES
     ApplicationManager.cpp
     BGHash.cpp
     CoverArt.cpp
     BackgroundEffectHelper.cpp
     DashStyle.cpp
     DebugDBusInterface.cpp
     DecorationStyle.cpp
     DefaultThumbnailProvider.cpp
     DeltaRestrainment.cpp
     DesktopApplicationManager.cpp
     EMConverter.cpp
     GnomeFileManager.cpp
     FontSettings.cpp
     GraphicsUtils.cpp
     IMTextEntry.cpp
     IconLoader.cpp
     IconRenderer.cpp
     IconTexture.cpp
     IconTextureSource.cpp
     Introspectable.cpp
     IntrospectableWrappers.cpp
     IntrospectionData.cpp
     JSONParser.cpp
     LayoutSystem.cpp
     LineSeparator.cpp
     OverlayRenderer.cpp
     OverlayWindowButtons.cpp
     PanelStyle.cpp
     PlacesVScrollBar.cpp
     PlacesOverlayVScrollBar.cpp
     PreviewStyle.cpp
     RatingsButton.cpp
     ResizingBaseWindow.cpp
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
     VScrollBarOverlayWindow.cpp
     WindowButtons.cpp
     WindowManager.cpp
)

if(ENABLE_X_SUPPORT)
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
add_pch(pch/unity-shared_pch.hh unity-shared)

#
# We also need to build compiz specific parts and standalone variants of those parts
#

if (ENABLE_X_SUPPORT)
  # compiz

  set (UNITY_SHARED_COMPIZ_SOURCES
    CompizUtils.cpp
    PluginAdapter.cpp
    )

  find_package (PkgConfig)
  pkg_check_modules (COMPIZ_OPENGL REQUIRED compiz-opengl)

  add_library (unity-shared-compiz STATIC ${UNITY_SHARED_COMPIZ_SOURCES})

  # This makes linker to include library dir in RUNPATH
  find_library (COMPIZ_OPENGL_LIB opengl ${COMPIZ_OPENGL_LIBDIR})
  target_link_libraries (unity-shared-compiz ${LIBS} ${COMPIZ_OPENGL_LIB} ${COMPIZ_OPENGL_LDFLAGS})
  add_dependencies (unity-shared-compiz unity-shared)

  # bamf application manager
  set (UNITY_SHARED_BAMF_SOURCES
    DesktopApplicationManager.cpp
    BamfApplicationManager.cpp
    BamfApplicationManagerFactory.cpp
    )
  add_library (unity-shared-bamf STATIC ${UNITY_SHARED_BAMF_SOURCES})
  target_link_libraries (unity-shared-bamf ${LIBS})
  add_dependencies (unity-shared-bamf unity-shared)
endif()

# standalone
set (UNITY_SHARED_STANDALONE_SOURCES
     StandaloneWindowManager.cpp
     )
add_library (unity-shared-standalone STATIC ${UNITY_SHARED_STANDALONE_SOURCES})
target_link_libraries (unity-shared-standalone ${LIBS})
add_dependencies (unity-shared-standalone unity-shared)

add_executable (app-manager StandaloneAppManager.cpp)
add_dependencies (app-manager  unity-shared unity-shared-bamf unity-shared-standalone)
target_link_libraries (app-manager  unity-shared unity-shared-bamf  unity-shared-standalone)

add_executable (decorations-style StandaloneDecorationStyle.cpp)
add_dependencies (decorations-style unity-shared)
target_link_libraries (decorations-style unity-shared)

