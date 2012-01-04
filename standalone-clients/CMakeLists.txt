set(UNITY_SRC ../plugins/unityshell/src)

#
# Data
#

#
# Unit tests
#
find_package (PkgConfig)
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};unity>=4.0.0")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS})

set (TESTDATADIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

set (CFLAGS
     ${TEST_UNIT_DEPS_CFLAGS}
     ${TEST_UNIT_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-g"
     "-DTESTDATADIR=\"${TESTDATADIR}\""
     "-DGETTEXT_PACKAGE=\"unity\""
     "-DINDICATORDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-DINDICATORICONDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-I${CMAKE_CURRENT_BINARY_DIR}"
     )
add_definitions (${CFLAGS})

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION} -lm")
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../${UNITY_SRC} or ../../services in alphabetically after that

#
# Standalone tests
#
add_executable (dash
                standalone_dash.cpp
                ${UNITY_SRC}/BackgroundEffectHelper.cpp
                ${UNITY_SRC}/BackgroundEffectHelper.h
                ${UNITY_SRC}/BGHash.cpp
                ${UNITY_SRC}/BGHash.h
                ${UNITY_SRC}/DashSearchBar.cpp
                ${UNITY_SRC}/DashSearchBar.h
                ${UNITY_SRC}/DashSearchBarSpinner.cpp
                ${UNITY_SRC}/DashSearchBarSpinner.h
                ${UNITY_SRC}/FilterExpanderLabel.cpp
                ${UNITY_SRC}/FilterFactory.cpp
                ${UNITY_SRC}/FilterBasicButton.cpp
                ${UNITY_SRC}/FilterRatingsButton.cpp
                ${UNITY_SRC}/FilterRatingsWidget.cpp
                ${UNITY_SRC}/FilterMultiRangeWidget.cpp
                ${UNITY_SRC}/FilterMultiRangeButton.cpp
                ${UNITY_SRC}/FilterGenreButton.cpp
                ${UNITY_SRC}/FilterGenreWidget.cpp
                ${UNITY_SRC}/FilterBar.cpp
                ${UNITY_SRC}/FontSettings.cpp
                ${UNITY_SRC}/FontSettings.h
                ${UNITY_SRC}/IMTextEntry.cpp
                ${UNITY_SRC}/IMTextEntry.h
                ${UNITY_SRC}/PlacesHomeView.cpp
                ${UNITY_SRC}/PlacesHomeView.h
                ${UNITY_SRC}/PlacesGroup.cpp
                ${UNITY_SRC}/PlacesGroup.h
                ${UNITY_SRC}/DashSettings.cpp
                ${UNITY_SRC}/DashSettings.h
                ${UNITY_SRC}/PlacesTile.cpp
                ${UNITY_SRC}/PlacesTile.h
                ${UNITY_SRC}/PlacesSimpleTile.cpp
                ${UNITY_SRC}/PlacesSimpleTile.h
                ${UNITY_SRC}/PlacesVScrollBar.cpp
                ${UNITY_SRC}/PlacesVScrollBar.h
                ${UNITY_SRC}/DashView.cpp
                ${UNITY_SRC}/DashView.h
                ${UNITY_SRC}/DashViewPrivate.cpp
                ${UNITY_SRC}/DashViewPrivate.h
                ${UNITY_SRC}/HomeView.cpp
                ${UNITY_SRC}/HomeView.h
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/IconLoader.h
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconTexture.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/LensBar.cpp
                ${UNITY_SRC}/LensBar.h
                ${UNITY_SRC}/LensBarIcon.cpp
                ${UNITY_SRC}/LensBarIcon.h
                ${UNITY_SRC}/LensView.cpp
                ${UNITY_SRC}/LensView.h
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/ResultView.cpp
                ${UNITY_SRC}/ResultViewGrid.cpp
                ${UNITY_SRC}/ResultRenderer.cpp
                ${UNITY_SRC}/ResultRendererTile.cpp
                ${UNITY_SRC}/ResultRendererHorizontalTile.cpp
                ${UNITY_SRC}/TextureCache.h
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/Timer.cpp
                ${UNITY_SRC}/Timer.h
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/StaticCairoText.h
                ${UNITY_SRC}/UBusMessages.h
                ${UNITY_SRC}/UBusWrapper.cpp
                ${UNITY_SRC}/UBusWrapper.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                ${UNITY_SRC}/UScreen.cpp
                ${UNITY_SRC}/UScreen.h
                )
add_dependencies (dash unity-core-${UNITY_API_VERSION})

add_executable (panel
                TestPanel.cpp
                ${UNITY_SRC}/Animator.cpp
                ${UNITY_SRC}/Animator.h
                ${UNITY_SRC}/BackgroundEffectHelper.cpp
                ${UNITY_SRC}/BackgroundEffectHelper.h
                ${UNITY_SRC}/DashSettings.cpp
                ${UNITY_SRC}/DashSettings.h
                ${UNITY_SRC}/PanelStyle.cpp
                ${UNITY_SRC}/PanelStyle.h
                ${UNITY_SRC}/PanelView.cpp
                ${UNITY_SRC}/PanelView.h
                ${UNITY_SRC}/PanelIndicatorsView.cpp
                ${UNITY_SRC}/PanelIndicatorsView.h
                ${UNITY_SRC}/PanelIndicatorEntryView.cpp
                ${UNITY_SRC}/PanelIndicatorEntryView.h
                ${UNITY_SRC}/PanelTitlebarGrabAreaView.h
                ${UNITY_SRC}/PanelTitlebarGrabAreaView.cpp
                ${UNITY_SRC}/PanelTray.cpp
                ${UNITY_SRC}/PanelTray.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/PanelMenuView.cpp
                ${UNITY_SRC}/PanelMenuView.h
                ${UNITY_SRC}/Timer.cpp
                ${UNITY_SRC}/Timer.h
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/StaticCairoText.h
                ${UNITY_SRC}/WindowButtons.cpp
                ${UNITY_SRC}/WindowButtons.h
                ${UNITY_SRC}/WindowManager.cpp
                ${UNITY_SRC}/WindowManager.h
                ${UNITY_SRC}/UScreen.cpp
                ${UNITY_SRC}/UScreen.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies (panel unity-core-${UNITY_API_VERSION})

add_executable (switcher
                TestSwitcher.cpp
                ${UNITY_SRC}/AbstractLauncherIcon.h
                ${UNITY_SRC}/AbstractIconRenderer.h
                ${UNITY_SRC}/SwitcherController.cpp
                ${UNITY_SRC}/SwitcherController.h
                ${UNITY_SRC}/SwitcherModel.cpp
                ${UNITY_SRC}/SwitcherModel.h
                ${UNITY_SRC}/SwitcherView.cpp
                ${UNITY_SRC}/SwitcherView.h
                ${UNITY_SRC}/LayoutSystem.h
                ${UNITY_SRC}/LayoutSystem.cpp
                ${UNITY_SRC}/WindowManager.h
                ${UNITY_SRC}/WindowManager.cpp
                ${UNITY_SRC}/IconRenderer.cpp
                ${UNITY_SRC}/IconRenderer.h
                ${UNITY_SRC}/MockLauncherIcon.h
                ${UNITY_SRC}/BackgroundEffectHelper.h
                ${UNITY_SRC}/BackgroundEffectHelper.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/StaticCairoText.h
                ${UNITY_SRC}/UBusMessages.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies (switcher unity-core-${UNITY_API_VERSION})

add_executable (launcher
                TestLauncher.cpp
                ${UNITY_SRC}/AbstractLauncherIcon.h
                ${UNITY_SRC}/AbstractIconRenderer.h
                ${UNITY_SRC}/LauncherIcon.cpp
                ${UNITY_SRC}/LauncherIcon.h
                ${UNITY_SRC}/SimpleLauncherIcon.cpp
                ${UNITY_SRC}/SimpleLauncherIcon.h
                ${UNITY_SRC}/BamfLauncherIcon.cpp
                ${UNITY_SRC}/BamfLauncherIcon.h
                ${UNITY_SRC}/BFBLauncherIcon.cpp
                ${UNITY_SRC}/BFBLauncherIcon.h
                ${UNITY_SRC}/SpacerLauncherIcon.cpp
                ${UNITY_SRC}/SpacerLauncherIcon.h
                ${UNITY_SRC}/DndData.cpp
                ${UNITY_SRC}/DndData.h
                ${UNITY_SRC}/TrashLauncherIcon.cpp
                ${UNITY_SRC}/TrashLauncherIcon.h
                ${UNITY_SRC}/DesktopLauncherIcon.cpp
                ${UNITY_SRC}/DesktopLauncherIcon.h
                ${UNITY_SRC}/DNDCollectionWindow.cpp
                ${UNITY_SRC}/DNDCollectionWindow.h
                ${UNITY_SRC}/DeviceLauncherIcon.cpp
                ${UNITY_SRC}/DeviceLauncherIcon.h
                ${UNITY_SRC}/DeviceLauncherSection.cpp
                ${UNITY_SRC}/DeviceLauncherSection.h
                ${UNITY_SRC}/DevicesSettings.cpp
                ${UNITY_SRC}/DevicesSettings.h
                ${UNITY_SRC}/FavoriteStore.cpp
                ${UNITY_SRC}/FavoriteStore.h
                ${UNITY_SRC}/FavoriteStoreGSettings.cpp
                ${UNITY_SRC}/FavoriteStoreGSettings.h
                ${UNITY_SRC}/LauncherEntryRemoteModel.cpp
                ${UNITY_SRC}/LauncherEntryRemoteModel.h
                ${UNITY_SRC}/LauncherEntryRemote.cpp
                ${UNITY_SRC}/LauncherEntryRemote.h
                ${UNITY_SRC}/LauncherController.cpp
                ${UNITY_SRC}/LauncherController.h
                ${UNITY_SRC}/LauncherModel.cpp
                ${UNITY_SRC}/LauncherModel.h
                ${UNITY_SRC}/Launcher.cpp
                ${UNITY_SRC}/Launcher.h
                ${UNITY_SRC}/LauncherHideMachine.cpp
                ${UNITY_SRC}/LauncherHideMachine.h
                ${UNITY_SRC}/LauncherHoverMachine.cpp
                ${UNITY_SRC}/LauncherHoverMachine.h
                ${UNITY_SRC}/LauncherDragWindow.cpp
                ${UNITY_SRC}/LauncherDragWindow.h
                ${UNITY_SRC}/GeisAdapter.cpp
                ${UNITY_SRC}/GeisAdapter.h
                ${UNITY_SRC}/WindowManager.h
                ${UNITY_SRC}/WindowManager.cpp
                ${UNITY_SRC}/IconRenderer.cpp
                ${UNITY_SRC}/IconRenderer.h
                ${UNITY_SRC}/Tooltip.cpp
                ${UNITY_SRC}/Tooltip.h
                ${UNITY_SRC}/BackgroundEffectHelper.h
                ${UNITY_SRC}/BackgroundEffectHelper.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/StaticCairoText.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/QuicklistMenuItem.cpp
                ${UNITY_SRC}/QuicklistMenuItem.h
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.cpp
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.h
                ${UNITY_SRC}/QuicklistMenuItemLabel.cpp
                ${UNITY_SRC}/QuicklistMenuItemLabel.h
                ${UNITY_SRC}/QuicklistMenuItemRadio.cpp
                ${UNITY_SRC}/QuicklistMenuItemRadio.h
                ${UNITY_SRC}/QuicklistMenuItemSeparator.cpp
                ${UNITY_SRC}/QuicklistMenuItemSeparator.h
                ${UNITY_SRC}/QuicklistView.cpp
                ${UNITY_SRC}/QuicklistView.h
                ${UNITY_SRC}/QuicklistManager.cpp
                ${UNITY_SRC}/QuicklistManager.h
                ${UNITY_SRC}/UBusMessages.h
                ${UNITY_SRC}/UBusWrapper.cpp
                ${UNITY_SRC}/UBusWrapper.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies (launcher unity-core-${UNITY_API_VERSION})

add_executable (keyutil
                TestKeyboardUtil.cpp
                ${UNITY_SRC}/KeyboardUtil.h
                ${UNITY_SRC}/KeyboardUtil.cpp
                )
add_dependencies (keyutil unity-core-${UNITY_API_VERSION})

add_executable (quicklist
                ui/TestQuicklist.cpp
                ui/EventFaker.cpp
                ui/EventFaker.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/QuicklistMenuItem.cpp
                ${UNITY_SRC}/QuicklistMenuItem.h
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.cpp
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.h
                ${UNITY_SRC}/QuicklistMenuItemLabel.cpp
                ${UNITY_SRC}/QuicklistMenuItemLabel.h
                ${UNITY_SRC}/QuicklistMenuItemRadio.cpp
                ${UNITY_SRC}/QuicklistMenuItemRadio.h
                ${UNITY_SRC}/QuicklistMenuItemSeparator.cpp
                ${UNITY_SRC}/QuicklistMenuItemSeparator.h
                ${UNITY_SRC}/QuicklistView.cpp
                ${UNITY_SRC}/QuicklistView.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies(quicklist unity-core-${UNITY_API_VERSION})

add_executable (quicklist-visuals
                ui/TestQuicklistVisuals.cpp
                ui/EventFaker.cpp
                ui/EventFaker.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/QuicklistMenuItem.cpp
                ${UNITY_SRC}/QuicklistMenuItem.h
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.cpp
                ${UNITY_SRC}/QuicklistMenuItemCheckmark.h
                ${UNITY_SRC}/QuicklistMenuItemLabel.cpp
                ${UNITY_SRC}/QuicklistMenuItemLabel.h
                ${UNITY_SRC}/QuicklistMenuItemRadio.cpp
                ${UNITY_SRC}/QuicklistMenuItemRadio.h
                ${UNITY_SRC}/QuicklistMenuItemSeparator.cpp
                ${UNITY_SRC}/QuicklistMenuItemSeparator.h
                ${UNITY_SRC}/QuicklistView.cpp
                ${UNITY_SRC}/QuicklistView.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies(quicklist-visuals unity-core-${UNITY_API_VERSION})

add_executable (filters
                TestFilters.cpp
                ${UNITY_SRC}/FilterExpanderLabel.cpp
                ${UNITY_SRC}/FilterBasicButton.cpp
                ${UNITY_SRC}/FilterRatingsButton.cpp
                ${UNITY_SRC}/FilterRatingsWidget.cpp
                ${UNITY_SRC}/FilterGenreButton.cpp
                ${UNITY_SRC}/FilterGenreWidget.cpp
                ${UNITY_SRC}/FilterMultiRangeButton.cpp
                ${UNITY_SRC}/FilterMultiRangeWidget.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                )
add_dependencies(filters unity-core-${UNITY_API_VERSION})

add_executable (filter-bar
                TestFilterBar.cpp
                ${UNITY_SRC}/FilterExpanderLabel.cpp
                ${UNITY_SRC}/FilterFactory.cpp
                ${UNITY_SRC}/FilterBasicButton.cpp
                ${UNITY_SRC}/FilterRatingsButton.cpp
                ${UNITY_SRC}/FilterRatingsWidget.cpp
                ${UNITY_SRC}/FilterMultiRangeWidget.cpp
                ${UNITY_SRC}/FilterMultiRangeButton.cpp
                ${UNITY_SRC}/FilterGenreButton.cpp
                ${UNITY_SRC}/FilterGenreWidget.cpp
                ${UNITY_SRC}/FilterBar.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                )
add_dependencies(filter-bar unity-core-${UNITY_API_VERSION})

add_executable (preview-applicaiton
                TestPreviewApplications.cpp
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/Timer.cpp
                )
add_dependencies(preview-applicaiton unity-core-${UNITY_API_VERSION})

add_executable (preview-generic
                TestPreviewGeneric.cpp
                ${UNITY_SRC}/PreviewGeneric.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/Timer.cpp
                )
add_dependencies(preview-generic unity-core-${UNITY_API_VERSION})

add_executable (preview-music
                TestPreviewMusic.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/PreviewMusic.cpp
                ${UNITY_SRC}/PreviewMusicTrack.cpp
                ${UNITY_SRC}/PreviewMusicTrackWidget.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/Timer.cpp
                )
add_dependencies(preview-music unity-core-${UNITY_API_VERSION})

add_executable (result-view
                TestResultView.cpp
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/ResultView.cpp
                ${UNITY_SRC}/ResultViewGrid.cpp
                ${UNITY_SRC}/ResultRenderer.cpp
                ${UNITY_SRC}/ResultRendererTile.cpp
                ${UNITY_SRC}/ResultRendererHorizontalTile.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/Timer.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/UBusMessages.h
                ${UNITY_SRC}/UBusWrapper.cpp
                ${UNITY_SRC}/UBusWrapper.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                )
add_dependencies(result-view unity-core-${UNITY_API_VERSION})

add_executable (dash-style
                TestDashStyle.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/DashStyle.h
                ${UNITY_SRC}/JSONParser.cpp
                ${PLACES_COMMON_SOURCE}
                )
add_dependencies(dash-style unity-core-${UNITY_API_VERSION})

add_executable (bg-hash
                TestBGHash.cpp
                ${UNITY_SRC}/BGHash.cpp
                ${UNITY_SRC}/BGHash.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
               )
add_dependencies (bg-hash unity-core-${UNITY_API_VERSION})

add_executable (hud
                StandaloneHud.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/JSONParser.cpp
                ${UNITY_SRC}/HudSearchBar.cpp
                ${UNITY_SRC}/HudButton.cpp
                ${UNITY_SRC}/DashStyle.cpp
                ${UNITY_SRC}/DashSearchBarSpinner.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/IMTextEntry.cpp
                ${UNITY_SRC}/HudView.cpp
                ${UNITY_SRC}/UBusWrapper.cpp
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/TextureCache.cpp
                ${UNITY_SRC}/Timer.cpp
               )
add_dependencies (hud unity-core-${UNITY_API_VERSION})


# Custom target to make all the other targets here, add your test to this list
add_custom_target(standalone-clients DEPENDS dash panel launcher switcher keyutil quicklist quicklist-visuals filters filter-bar preview-applicaiton preview-generic preview-music result-view dash-style bg-hash)




