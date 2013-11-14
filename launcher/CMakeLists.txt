set(UNITY_SRC ../plugins/unityshell/src)

set (CFLAGS
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     "-fPIC"
     ${PIC_FLAGS}
     )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")

set (LIBS ${CACHED_UNITY_DEPS_LDFLAGS} ${UNITY_STANDALONE_LADD})

include_directories (.. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

#
# Headers & Sources
#
set (LAUNCHER_SOURCES
     AbstractLauncherIcon.cpp
     ApplicationLauncherIcon.cpp
     BFBLauncherIcon.cpp
     CairoBaseWindow.cpp
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
     TooltipManager.cpp
     TrashLauncherIcon.cpp
     VolumeImp.cpp
     VolumeLauncherIcon.cpp
     VolumeMonitorWrapper.cpp
     XdndCollectionWindowImp.cpp
     XdndManagerImp.cpp
     XdndStartStopNotifier.cpp
     XdndStartStopNotifierImp.cpp
     )

if (ENABLE_X_SUPPORT)
  set (LAUNCHER_SOURCES
    EdgeBarrierController.cpp
    PointerBarrier.cpp
    ${LAUNCHER_SOURCES}
    )
endif ()

add_library (launcher-lib STATIC ${LAUNCHER_SOURCES})
add_dependencies (launcher-lib unity-core-${UNITY_API_VERSION} unity-shared)
target_link_libraries (launcher-lib unity-shared)
add_pch(pch/launcher_pch.hh launcher-lib)

set (SWITCHER_SOURCES
      DeltaTracker.cpp
      SwitcherController.cpp
      SwitcherModel.cpp
      SwitcherView.cpp
    )

add_library (switcher-lib STATIC ${SWITCHER_SOURCES})
add_dependencies (switcher-lib unity-core-${UNITY_API_VERSION} unity-shared)

set (LAUNCHER_LIBS
    launcher-lib
    unity-shared
    unity-shared-bamf
    unity-shared-standalone)

#
# Standalone variant
#
add_executable (launcher StandaloneLauncher.cpp)
target_link_libraries (launcher ${LAUNCHER_LIBS})

if (ENABLE_X_SUPPORT)
  add_executable (switcher StandaloneSwitcher.cpp)
  target_link_libraries (switcher switcher-lib ${LAUNCHER_LIBS})
endif ()
