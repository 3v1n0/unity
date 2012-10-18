set(UNITY_SRC ../plugins/unityshell/src)

find_package (PkgConfig)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     "-fPIC"
     )

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
  set (CFLAGS ${CFLAGS} "-fPIC")
endif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

add_definitions (${CFLAGS})

set (LIBS ${CACHED_UNITY_DEPS_LIBRARIES} ${UNITY_STANDALONE_LADD})

link_libraries (${LIBS})

set (LIB_PATHS ${CACHED_UNITY_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (.. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

#
# Headers & Sources
#
set (LAUNCHER_SOURCES
     AbstractLauncherIcon.cpp
     ApplicationLauncherIcon.cpp
     BFBLauncherIcon.cpp
     CairoBaseWindow.cpp
     DNDCollectionWindow.cpp
     Decaymulator.cpp
     DesktopLauncherIcon.cpp
     DeviceLauncherSection.cpp
     DeviceNotificationDisplayImp.cpp
     DevicesSettingsImp.cpp
     DndData.cpp
     ExpoLauncherIcon.cpp
     FavoriteStore.cpp
     FavoriteStoreGSettings.cpp
     FavoriteStorePrivate.cpp
     FileManagerOpenerImp.cpp
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
     VolumeImp.cpp
     VolumeLauncherIcon.cpp
     VolumeMonitorWrapper.cpp
     )

if(UNITY_ENABLE_X_ORG_SUPPORT)
  set (LAUNCHER_SOURCES
    EdgeBarrierController.cpp
    PointerBarrier.cpp
    ${LAUNCHER_SOURCES}
    )
endif()

add_library (launcher-lib STATIC ${LAUNCHER_SOURCES})
add_dependencies (launcher-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (launcher-lib unity-shared)

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
target_link_libraries (launcher launcher-lib unity-shared unity-shared-standalone)

if(UNITY_ENABLE_X_ORG_SUPPORT)
  add_executable (switcher StandaloneSwitcher.cpp)
  target_link_libraries (switcher switcher-lib launcher-lib unity-shared unity-shared-standalone)
endif()
