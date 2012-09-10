set(UNITY_SRC ../plugins/unityshell/src)

#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/ubuntuone-installer.desktop
                ${CMAKE_BINARY_DIR}/tests/data/ubuntuone-installer.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/ubuntu-software-center.desktop
                ${CMAKE_BINARY_DIR}/tests/data/ubuntu-software-center.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/update-manager.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/bzr-handle-patch.desktop
                ${CMAKE_BINARY_DIR}/tests/data/bzr-handle-patch.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/no-icon.desktop
                ${CMAKE_BINARY_DIR}/tests/data/no-icon.desktop)
#
# Unit tests
#
find_package (PkgConfig)
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};unity>=4.99.0")
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

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION} -lcompiz_core -lm")
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS} ${COMPIZ_LIBDIR})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})
include_directories (${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src)
include_directories (${CMAKE_SOURCE_DIR}/dash)
include_directories (${CMAKE_SOURCE_DIR}/launcher)
include_directories (${CMAKE_SOURCE_DIR}/panel)
include_directories (${CMAKE_SOURCE_DIR}/hud)
include_directories (${CMAKE_SOURCE_DIR}/shortcuts)
include_directories (${CMAKE_SOURCE_DIR}/unity-shared)
# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../${UNITY_SRC} or ../../services in alphabetically after that
find_program(GLIB_GENMARSHAL glib-genmarshal)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/panel-marshal.c
  COMMAND ${GLIB_GENMARSHAL} ARGS ../services/panel-marshal.list --body --prefix=panel_marshal > ${CMAKE_CURRENT_BINARY_DIR}/panel-marshal.c
  COMMAND ${GLIB_GENMARSHAL} ARGS ../services/panel-marshal.list --header --prefix=panel_marshal > ${CMAKE_CURRENT_BINARY_DIR}/panel-marshal.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ../services/panel-marshal.list
  COMMENT "Generating marshallers for panel-service")

add_executable (test-unit
                unit/TestMain.cpp
                unit/TestPanelService.cpp
                unit/TestUBus.cpp
                unit/TestStaticCairoText.cpp
                ${CMAKE_SOURCE_DIR}/launcher/CairoBaseWindow.cpp
                ${CMAKE_SOURCE_DIR}/unity-shared/Introspectable.cpp
                ${CMAKE_SOURCE_DIR}/unity-shared/StaticCairoText.cpp
                ../services/panel-service.c
                ${CMAKE_CURRENT_BINARY_DIR}/panel-marshal.c
                ${CMAKE_SOURCE_DIR}/unity-shared/ubus-server.cpp
                )
target_link_libraries (test-unit ${LIBS})
add_dependencies (test-unit unity-core-${UNITY_API_VERSION})
add_subdirectory (test-input-remover)
add_subdirectory (test-minimize-window-handler)
add_subdirectory (test-get-transients)

#
# GTest tests
#
find_program(GLIB_GENMARSHAL glib-genmarshal)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
  COMMAND ${GLIB_GENMARSHAL} ARGS test_glib_signals_utils_marshal.list --body --prefix=test_signals > ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
  COMMAND ${GLIB_GENMARSHAL} ARGS test_glib_signals_utils_marshal.list --header --prefix=test_signals > ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.h
  COMMAND sed ARGS -i "s/glib-object/test_glib_signals_utils_marshal/" ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS test_glib_signals_utils_marshal.list
  COMMENT "Generating marshallers")

enable_testing()
# :(
find_library (GMOCK_LIB gmock)
find_library (GMOCK_MAIN_LIB gmock_main)
find_path(GTEST_SRC_DIR gtest.cc PATHS /usr/src/gtest/src)

if (GTEST_SRC_DIR AND
    GMOCK_LIB AND
    GMOCK_MAIN_LIB)

# The service that provides DBus services to test against
  add_executable(test-gtest-service
                 test_service_gdbus_wrapper.c
                 test_service_gdbus_wrapper.h
                 test_service_hud.c
                 test_service_hud.h
                 test_service_lens.c
                 test_service_lens.h
                 test_service_main.c
                 test_service_model.c
                 test_service_model.h)
  target_link_libraries(test-gtest-service ${LIBS})
  add_dependencies (test-gtest-service unity-core-${UNITY_API_VERSION} gtest)


# The actual test executable (xless) - do not put anything that requires X in here
  add_executable(test-gtest-xless
                 test_main_xless.cpp
                 test_animator.cpp
                 test_launcher_model.cpp
                 test_glib_object.cpp
                 test_glib_object_utils.cpp
                 test_glib_object_utils.h
                 test_glib_signals.cpp
                 test_glib_signals_utils.cpp
                 test_glib_signals_utils.h
                 test_glib_source.cpp
                 test_glib_variant.cpp
                 test_grabhandle.cpp
                 test_desktop_utilities.cpp
                 test_hud_private.cpp
                 test_indicator.cpp
                 test_indicator_appmenu.cpp
                 test_indicator_entry.cpp
                 test_indicators.cpp
                 test_introspection.cpp
                 test_favorite_store_gsettings.cpp
                 test_favorite_store_private.cpp
                 test_home_lens.cpp
                 test_launcher_entry_remote.cpp
                 test_model_iterator.cpp
                 test_pointer_barrier.cpp
                 test_previews.cpp
                 test_shortcut_model.cpp
                 test_shortcut_private.cpp
                 test_showdesktop_handler.cpp
                 test_unityshell_private.cpp
                 ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/AbstractLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Animator.cpp
                 ${UNITY_SRC}/DebugDBusInterface.cpp
                 ${UNITY_SRC}/XPathQueryPart.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStore.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStoreGSettings.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IconTextureSource.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherModel.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherEntryRemote.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/MockLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStorePrivate.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/PointerBarrier.cpp
                 ${CMAKE_SOURCE_DIR}/shortcuts/ShortcutModel.cpp
                 ${CMAKE_SOURCE_DIR}/shortcuts/ShortcutHintPrivate.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Introspectable.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/TextureCache.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Timer.cpp
                 ${UNITY_SRC}/UnityshellPrivate.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/WindowManager.cpp
                 ${UNITY_SRC}/UnityShowdesktopHandler.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-group.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-impl-factory.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-layout.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-texture.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudPrivate.cpp
                 )
  target_link_libraries(test-gtest-xless gtest ${GMOCK_LIB} ${GMOCK_MAIN_LIB} ${LIBS})
  add_test(UnityGTestXless test-gtest-xless)
  add_dependencies(test-gtest-xless unity-core-${UNITY_API_VERSION} gtest)

# tests that require dbus, must not require X
  add_executable(test-gtest-dbus
                 test_categories.cpp
                 test_filesystem_lenses.cpp
                 test_filter.cpp
                 test_gdbus_proxy.cpp
                 test_hud.cpp
                 test_lens.cpp
                 test_main_dbus.cpp
                 test_model.cpp
                 test_utils.h
                 test_ratings_filter.cpp
                 test_results.cpp
                )
  target_link_libraries(test-gtest-dbus gtest ${LIBS})
  add_test(UnityGTestDBus test-gtest-dbus)
  add_dependencies(test-gtest-dbus unity-core-${UNITY_API_VERSION} test-gtest-service gtest)

# Tests that require X
  add_executable(test-gtest
                 test_main.cpp
                 test_bamf_launcher_icon.cpp
                 test_bfb_launcher_icon.cpp
                 test_dashview_impl.cpp
                 test_desktop_launcher_icon.cpp
                 test_edge_barrier_controller.cpp
                 test_launcher.cpp
                 test_device_launcher_section.cpp
                 test_lensview_impl.cpp
                 test_hud_button.cpp
                 test_hud_controller.cpp
                 test_hud_launcher_icon.cpp
                 test_hud_view.cpp
                 test_icon_loader.cpp
                 test_im_text_entry.cpp
                 test_launcher_controller.cpp
                 test_launcher_drag_window.cpp
                 test_keyboard_util.cpp
                 test_panel_style.cpp
                 test_previews_application.cpp
                 test_previews_generic.cpp
                 test_previews_movie.cpp
                 test_previews_music.cpp
                 test_quicklist_menu_item.cpp
                 test_quicklist_view.cpp
                 test_resultviewgrid.cpp
                 test_single_monitor_launcher_icon.cpp
                 test_expo_launcher_icon.cpp
                 test_switcher_controller.cpp
                 test_switcher_model.cpp
                 test_texture_cache.cpp
                 test_thumbnail_generator.cpp
                 test_launcher_minimize_speed.cpp
                 test_volume_imp.cpp
                 test_volume_launcher_icon.cpp
                 gmockmount.c
                 gmockvolume.c
                 ${CMAKE_SOURCE_DIR}/dash/AbstractPlacesGroup.cpp
                 ${CMAKE_SOURCE_DIR}/dash/DashViewPrivate.cpp
                 ${CMAKE_SOURCE_DIR}/dash/LensViewPrivate.cpp
                 ${CMAKE_SOURCE_DIR}/dash/ResultRenderer.cpp
                 ${CMAKE_SOURCE_DIR}/dash/ResultView.cpp
                 ${CMAKE_SOURCE_DIR}/dash/ResultViewGrid.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/ActionButton.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/ApplicationPreview.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/GenericPreview.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/MoviePreview.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/MusicPreview.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/Preview.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/PreviewInfoHintWidget.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/PreviewRatingsWidget.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/Tracks.cpp
                 ${CMAKE_SOURCE_DIR}/dash/previews/Track.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudAbstractView.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudButton.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudController.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudIcon.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudIconTextureSource.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudPrivate.cpp
                 ${CMAKE_SOURCE_DIR}/hud/HudView.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/AbstractLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/BamfLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/BFBLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/CairoBaseWindow.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DNDCollectionWindow.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/Decaymulator.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DesktopLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DeviceLauncherSection.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DeviceNotificationDisplayImp.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DevicesSettingsImp.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/DndData.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/EdgeBarrierController.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/ExpoLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStore.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStoreGSettings.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FavoriteStorePrivate.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/FileManagerOpenerImp.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/HudLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/Launcher.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherController.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherDragWindow.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherEntryRemote.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherEntryRemoteModel.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherHideMachine.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherHoverMachine.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherModel.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LauncherOptions.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/LayoutSystem.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/MockLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/PointerBarrier.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistManager.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistMenuItem.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistMenuItemCheckmark.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistMenuItemLabel.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistMenuItemRadio.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistMenuItemSeparator.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/QuicklistView.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SimpleLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SingleMonitorLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SoftwareCenterLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SpacerLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SwitcherController.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SwitcherModel.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/SwitcherView.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/Tooltip.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/TrashLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/VolumeImp.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/VolumeLauncherIcon.cpp
                 ${CMAKE_SOURCE_DIR}/launcher/VolumeMonitorWrapper.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Animator.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/BackgroundEffectHelper.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/CoverArt.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/DashStyle.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/DefaultThumbnailProvider.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IconLoader.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IconRenderer.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IconTexture.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IconTextureSource.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IMTextEntry.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Introspectable.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/IntrospectableWrappers.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/JSONParser.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/KeyboardUtil.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/OverlayRenderer.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/PanelStyle.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/PlacesVScrollBar.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/PluginAdapterStandalone.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/PreviewStyle.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/RatingsButton.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/SearchBar.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/SearchBarSpinner.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/StaticCairoText.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/TextureCache.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/Timer.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/TextureThumbnailProvider.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/ThumbnailGenerator.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UBusWrapper.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UScreen.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UnitySettings.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UnityWindowStyle.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UnityWindowView.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/UserThumbnailProvider.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/WindowManager.cpp
                 ${CMAKE_SOURCE_DIR}/unity-shared/ubus-server.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unityshell/src/WindowMinimizeSpeedController.cpp
                )
  target_link_libraries(test-gtest gtest gmock ${LIBS})
  add_test(UnityGTest test-gtest)
  add_dependencies(test-gtest unity-core-${UNITY_API_VERSION} gtest gmock)

endif (GTEST_SRC_DIR AND
       GMOCK_LIB AND
       GMOCK_MAIN_LIB)

add_subdirectory (test-gestures)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (GTEST_TEST_COMMAND ./test-gtest)
set (GTEST_TEST_COMMAND_XLESS ./test-gtest-xless)
set (GTEST_TEST_COMMAND_DBUS dbus-test-runner --task ./test-gtest-service --task ./test-gtest-dbus)
set (GTEST_TEST_COMMAND_GESTURES ./test-gestures/test-gestures)

set (TEST_COMMAND
     gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit
  && ${GTEST_TEST_COMMAND} --gtest_output=xml:./
  && ${GTEST_TEST_COMMAND_XLESS} --gtest_output=xml:./
  && ${GTEST_TEST_COMMAND_GESTURES} --gtest_output=xml:./
  && ${GTEST_TEST_COMMAND_DBUS} --gtest_output=xml:./
    )

set (TEST_COMMAND_HEADLESS
     ${GTEST_TEST_COMMAND_XLESS} --gtest_output=xml:./
  && ${GTEST_TEST_COMMAND_GESTURES} --gtest_output=xml:./
  #&& ${GTEST_TEST_COMMAND_DBUS} --gtest_output=xml:./
  && echo "Warning, DBus test cases are disabled!!")

if (GTEST_SRC_DIR)
  add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest test-gtest-xless test-gtest-dbus test-gestures)
  add_custom_target (check-headless COMMAND ${TEST_COMMAND_HEADLESS} DEPENDS test-gtest-xless test-gtest-dbus test-gestures)
  add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
  add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest test-gtest-xless)
else (GTEST_SRC_DIR)
  add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit)
endif (GTEST_SRC_DIR)
# make target to allow devs to run "make autopilot" from build dir:
set (AUTOPILOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/autopilot")
# Rules to install autopilot files and executable script:
install(CODE "execute_process(COMMAND python setup.py install --prefix ${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${AUTOPILOTDIR})")
add_custom_target (autopilot COMMAND cd ${AUTOPILOTDIR} && make check)
