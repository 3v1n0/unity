set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-fPIC"
     "-DGETTEXT_PACKAGE=\"unity\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
  set (CFLAGS ${CFLAGS} "-fPIC")
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} ${UNITY_STANDALONE_LADD})

link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

#
# Headers & Sources
#
set (LAUNCHER_SOURCES
     AbstractLauncherIcon.cpp
     BFBLauncherIcon.cpp
     BamfLauncherIcon.cpp
     CairoBaseWindow.cpp
     DNDCollectionWindow.cpp
     Decaymulator.cpp
     DesktopLauncherIcon.cpp
     VolumeLauncherIcon.cpp
     DeviceLauncherSection.cpp
     DevicesSettings.cpp
     DndData.cpp
     EdgeBarrierController.cpp
     FavoriteStore.cpp
     FavoriteStoreGSettings.cpp
     FavoriteStorePrivate.cpp
     HudLauncherIcon.cpp
     Launcher.cpp
     LauncherController.cpp
     LauncherDragWindow.cpp
     LauncherEntryRemote.cpp
     LauncherEntryRemoteModel.cpp
     LauncherHideMachine.cpp
     LauncherHoverMachine.cpp
     LauncherIcon.cpp
     LauncherModel.cpp
     LauncherOptions.cpp
     MockLauncherIcon.cpp
     PointerBarrier.cpp
     QuicklistManager.cpp
     QuicklistMenuItem.cpp
     QuicklistMenuItemCheckmark.cpp
     QuicklistMenuItemLabel.cpp
     QuicklistMenuItemRadio.cpp
     QuicklistMenuItemSeparator.cpp
     QuicklistView.cpp
     SimpleLauncherIcon.cpp
     SingleMonitorLauncherIcon.cpp
     SoftwareCenterLauncherIcon.cpp
     SpacerLauncherIcon.cpp
     Tooltip.cpp
     TrashLauncherIcon.cpp
     VolumeMonitorWrapper.cpp
     )

add_library (launcher-lib STATIC ${LAUNCHER_SOURCES})
add_dependencies (launcher-lib unity-core-${UNITY_API_VERSION} unity-shared)

set (SWITCHER_SOURCES
      SwitcherController.cpp
      SwitcherModel.cpp
      SwitcherView.cpp
      LayoutSystem.cpp
    )

add_library (switcher-lib STATIC ${SWITCHER_SOURCES})
add_dependencies (switcher-lib unity-core-${UNITY_API_VERSION} unity-shared)

#
# Standalone variant
#
add_executable (launcher StandaloneLauncher.cpp)
add_dependencies (launcher launcher-lib)
target_link_libraries (launcher launcher-lib unity-shared)

add_executable (switcher StandaloneSwitcher.cpp)
add_dependencies (switcher switcher-lib launcher-lib)
target_link_libraries (switcher switcher-lib launcher-lib unity-shared)
