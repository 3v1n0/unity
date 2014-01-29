set(UNITY_SRC ../plugins/unityshell/src)

#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/ubuntuone-installer.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/ubuntuone-installer.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/ubuntu-software-center.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/ubuntu-software-center.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/update-manager.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/bzr-handle-patch.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/bzr-handle-patch.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/no-icon.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/no-icon.desktop)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/applications/kde4/afile.desktop
                ${CMAKE_BINARY_DIR}/tests/data/applications/kde4/afile.desktop)

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/scopes/testscope1.scope
                ${CMAKE_BINARY_DIR}/tests/data/unity/scopes/testscope1.scope)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/scopes/testscope2.scope
                ${CMAKE_BINARY_DIR}/tests/data/unity/scopes/testscope2.scope)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/scopes/testscope3.scope
                ${CMAKE_BINARY_DIR}/tests/data/unity/scopes/testscope3.scope)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/scopes/testscope4.scope
                ${CMAKE_BINARY_DIR}/tests/data/unity/scopes/testscope4.scope)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/scopes/commands.scope
                ${CMAKE_BINARY_DIR}/tests/data/unity/scopes/commands.scope)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/sounds/brownnoise.mp3
                ${CMAKE_BINARY_DIR}/tests/data/unity/sounds/brownnoise.mp3 COPYONLY)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/sounds/pinknoise.mp3
                ${CMAKE_BINARY_DIR}/tests/data/unity/sounds/pinknoise.mp3 COPYONLY)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/unity/sounds/whitenoise.mp3
                ${CMAKE_BINARY_DIR}/tests/data/unity/sounds/whitenoise.mp3 COPYONLY)

#
# Unit tests
#
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};unity>=4.99.0;libupstart;libnih;libnih-dbus;dbus-1")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS})

string (REPLACE ";" " " TEST_UNIT_DEPS_CFLAGS_OTHER "${TEST_UNIT_CFLAGS_OTHER}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TEST_UNIT_DEPS_CFLAGS_OTHER}")
include_directories(${TEST_UNIT_DEPS_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set (LIBS ${TEST_UNIT_DEPS_LDFLAGS} "unity-core-${UNITY_API_VERSION};compiz_core;m")

link_directories(${TEST_UNIT_DEPS_LIBRARY_DIRS} ${COMPIZ_LIBDIR})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})
include_directories (${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src)
include_directories (${CMAKE_SOURCE_DIR}/dash)
include_directories (${CMAKE_SOURCE_DIR}/launcher)
include_directories (${CMAKE_SOURCE_DIR}/panel)
include_directories (${CMAKE_SOURCE_DIR}/hud)
include_directories (${CMAKE_SOURCE_DIR}/shortcuts)
include_directories (${CMAKE_SOURCE_DIR}/shutdown)
include_directories (${CMAKE_SOURCE_DIR}/unity-shared)
# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../${UNITY_SRC} or ../../services in alphabetically after that

add_subdirectory (test-input-remover)
add_subdirectory (test-minimize-window-handler)
add_subdirectory (test-get-transients)


#
# GTest tests
#

enable_testing()
set(GMOCK_LIB gmock)
set(GMOCK_MAIN_LIB gmock_main)

if (GMOCK_LIB AND
    GMOCK_MAIN_LIB)

  # MockWindowManager
  add_library (unity_mock_window_manager STATIC
               MockWindowManager.cpp)
  target_link_libraries (unity_mock_window_manager
                         ${GMOCK_LIB}
                         ${GMOCK_MAIN_LIB})

# The service that provides DBus services to test against
  add_executable(test-gtest-service
                 test_service_gdbus_wrapper.cpp
                 test_service_hud.cpp
                 test_service_main.cpp
                 test_service_model.cpp
                 test_service_panel.cpp
                 test_service_scope.cpp)

  # Build plain C files separately so they don't try to include the
  # C++ pch.
  add_library(test-gtest-service-plainc STATIC
              test_scope_impl.c)

  target_link_libraries(test-gtest-service
                        test-gtest-service-plainc
                        unity-shared
                        ${LIBS})

# gtest-slow, start moving things over that are slow running tests
  set (GTEST_SLOW_SOURCES
       test_main.cpp
       logger_helper.cpp
       test_switcher_controller_slow.cpp
       test_switcher_controller_class.cpp
       test_tooltip_manager.cpp
      )

  set (GTEST_SLOW_LIBS
       gtest
       gmock
       launcher-lib
       switcher-lib
       unity-shared
       unity-shared-standalone
      )

  add_executable(test-gtest-slow ${GTEST_SLOW_SOURCES})
  target_link_libraries(test-gtest-slow ${GTEST_SLOW_LIBS})
  add_test(UnityGTestSlow test-gtest-slow)

# The actual test executable (xless) - do not put anything that requires X in here
  set (GTEST_XLESS_SOURCES
       test_main_xless.cpp
       mock-application.cpp
       test_action_handle.cpp
       test_abstract_interface_generator.cpp
       test_animation_utils.cpp
       test_connection_manager.cpp
       test_delta_tracker.cpp
       test_em_converter.cpp
       test_glib_dbus_object.cpp
       test_glib_cancellable.cpp
       test_glib_object.cpp
       test_glib_object_utils.cpp
       test_glib_object_utils.h
       test_glib_signals.cpp
       test_glib_signals_utils.cpp
       test_glib_signals_utils.h
       test_glib_source.cpp
       test_glib_variant.cpp
       test_grabhandle.cpp
       test_gsettings_scopes.cpp
       test_desktop_utilities.cpp
       test_desktop_application_subject.cpp
       test_indicator.cpp
       test_indicator_appmenu.cpp
       test_indicator_entry.cpp
       test_indicators.cpp
       test_introspection_data.cpp
       test_favorite_store.cpp
       test_favorite_store_gsettings.cpp
       test_favorite_store_private.cpp
       test_launcher_entry_remote.cpp
       test_launcher_model.cpp
       test_launcher_options.cpp
       test_layout_system.cpp
       test_model_iterator.cpp
       test_previews.cpp
       test_scope_data.cpp
       test_time_util.cpp
       test_ubus.cpp
       test_unityshell_private.cpp
       test_volume_imp.cpp
       ${UNITY_SRC}/UnityshellPrivate.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-group.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-impl-factory.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-layout.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-texture.cpp
  )

  set (GTEST_XLESS_LIBS
      gtest
      test-libs-c
      unity-shared
      unity-shared-standalone
      launcher-lib
      switcher-lib
      ${GMOCK_LIB}
      ${GMOCK_MAIN_LIB}
      ${LIBS}
  )

  if (ENABLE_X_SUPPORT)
    set (GTEST_XLESS_SOURCES
      ${GTEST_XLESS_SOURCES}
      test_hud_private.cpp
      test_pointer_barrier.cpp
      test_shortcut_model.cpp
      test_shortcut_private.cpp
      ${LAUNCHER_SOURCES}
      )
    set (GTEST_XLESS_LIBS
      ${GTEST_XLESS_LIBS}
      shortcuts-lib
      hud-lib
    )
  endif ()

  add_executable(test-gtest-xless ${GTEST_XLESS_SOURCES})
  target_link_libraries(test-gtest-xless ${GTEST_XLESS_LIBS})
  add_test(UnityGTestXless test-gtest-xless)

# tests that require dbus, must not require X
  add_executable(test-gtest-dbus
                 test_categories.cpp
                 test_dbus_indicators.cpp
                 test_filter.cpp
                 test_glib_dbus_proxy.cpp
                 test_hud.cpp
                 test_main_dbus.cpp
                 test_model.cpp
                 test_utils.h
                 test_ratings_filter.cpp
                 test_results.cpp
                 test_scope.cpp
                 test_scope_filter.cpp
                 test_scope_proxy.cpp
                 test_tracks.cpp
                )
  target_link_libraries(test-gtest-dbus gtest unity-shared ${LIBS})
  add_test(UnityGTestDBus test-gtest-dbus)
  add_dependencies(test-gtest-dbus unity-core-${UNITY_API_VERSION} unity-shared test-gtest-service gtest)

if (ENABLE_X_SUPPORT)
# Tests that require X
  add_executable(test-gtest
                 mock-application.cpp
                 mock_results.cpp
                 logger_helper.cpp
                 test_main.cpp
                 test_action_link.cpp
                 test_application_launcher_icon.cpp
                 test_bamf_application.cpp
                 test_bfb_launcher_icon.cpp
                 test_dashview.cpp
                 test_dashview_impl.cpp
                 test_dash_controller.cpp
                 test_desktop_launcher_icon.cpp
                 test_device_launcher_section.cpp
                 test_error_preview.cpp
                 test_edge_barrier_controller.cpp
                 test_expo_launcher_icon.cpp
                 test_filter_widgets.cpp
                 test_glib_dbus_server.cpp
                 test_gnome_session_manager.cpp
                 test_gtk_wrapper.cpp
                 test_hud_button.cpp
                 test_hud_controller.cpp
                 test_hud_launcher_icon.cpp
                 test_hud_view.cpp
                 test_icon_loader.cpp
                 test_im_text_entry.cpp
                 test_keyboard_util.cpp
                 test_launcher.cpp
                 test_launcher_controller.cpp
                 test_launcher_drag_window.cpp
                 test_launcher_hide_machine.cpp
                 test_launcher_hover_machine.cpp
                 test_launcher_icon.cpp
                 test_launcher_minimize_speed.cpp
                 test_launcher_tooltip.cpp
                 test_lockscreen_controller.cpp
                 test_panel_controller.cpp
                 test_panel_indicators_view.cpp
                 test_panel_menu_view.cpp
                 test_panel_service.cpp
                 test_panel_style.cpp
                 test_panel_tray.cpp
                 test_panel_view.cpp
                 test_places_group.cpp
                 test_preview_player.cpp
                 test_previews_application.cpp
                 test_previews_generic.cpp
                 test_previews_movie.cpp
                 test_previews_music.cpp
                 test_previews_music_payment.cpp
                 test_previews_payment.cpp
                 test_previews_social.cpp
                 test_overlay_scrollbar.cpp
                 test_quicklist_manager.cpp
                 test_quicklist_menu_item.cpp
                 test_quicklist_view.cpp
                 test_result_renderer.cpp
                 test_resultviewgrid.cpp
                 test_scope_bar.cpp
                 test_scope_view.cpp
                 test_searchbar.cpp
                 test_session_button.cpp
                 test_session_controller.cpp
                 test_session_view.cpp
                 test_shortcut_controller.cpp
                 test_shortcut_modeller_compiz.cpp
                 test_shortcut_view.cpp
                 test_single_monitor_launcher_icon.cpp
                 test_showdesktop_handler.cpp
                 test_software_center_launcher_icon.cpp
                 test_static_cairo_text.cpp
                 test_switcher_controller.cpp
                 test_switcher_controller_class.cpp
                 test_switcher_model.cpp
                 test_switcher_view.cpp
                 test_tabiterator.cpp
                 test_texture_cache.cpp
                 test_text_input.cpp
                 test_thumbnail_generator.cpp
                 test_trash_launcher_icon.cpp
                 test_unity_settings.cpp
                 test_unity_window_style.cpp
                 test_unity_window_view.cpp
                 test_user_authenticator_pam.cpp
                 test_volume_launcher_icon.cpp
                 test_window_buttons.cpp
                 test_xdnd_manager_imp.cpp
                 test_xdnd_start_stop_notifier_imp.cpp
                 ${UNITY_SRC}/UnityShowdesktopHandler.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unityshell/src/WindowMinimizeSpeedController.cpp
                 ${CMAKE_SOURCE_DIR}/services/panel-service.c
                )

  # Build plain C files separately so they don't try to include the
  # C++ pch.
  add_library(test-libs-c STATIC
              bamf-mock-application.c
              bamf-mock-window.c
              gmockmount.c
              gmockvolume.c
              )
  target_link_libraries(test-gtest
                        test-libs-c
                        gtest
                        gmock
                        unity-shared
                        ${LIBS}
                        launcher-lib
                        lockscreen-lib
                        unity-shared-bamf
                        unity-shared-standalone
                        shortcuts-lib
                        previews-lib
                        hud-lib
                        switcher-lib
                        shutdown-lib
                        dash-lib
                        panel-lib
                        pam)
  add_test(UnityGTest test-gtest)
endif (ENABLE_X_SUPPORT)
endif (GMOCK_LIB AND
       GMOCK_MAIN_LIB)

if(ENABLE_X_SUPPORT)
  add_subdirectory (test-gestures)
endif()

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (DUMMY_XORG_TEST_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/dummy-xorg-test-runner.sh)
if (ENABLE_X_SUPPORT)
  set (GTEST_TEST_COMMAND ./test-gtest --gtest_output=xml:./test-gtest.xml)
  set (GTEST_TEST_COMMAND_GESTURES ./test-gestures/test-gestures --gtest_output=xml:./test-gestures.xml)
endif (ENABLE_X_SUPPORT)
set (GTEST_TEST_COMMAND_XLESS ./test-gtest-xless --gtest_output=xml:./test-gtest-xless.xml)
set (GTEST_TEST_COMMAND_SLOW ./test-gtest-slow --gtest_output=xml:./test-gtest-slow.xml)
set (GTEST_TEST_COMMAND_DBUS dbus-test-runner --max-wait=300 --task ./test-gtest-service --task-name test-service --task=./test-gtest-dbus --task-name=test-dbus --wait-for=com.canonical.Unity.Test.Scope --parameter=--gtest_output=xml:./test-gtest-dbus.xml --parameter=--gtest_filter=-TestCategoriesChanging*)

set (TEST_COMMAND_XLESS
     ${GTEST_TEST_COMMAND_XLESS}
  && ${GTEST_TEST_COMMAND_GESTURES}
  && ${GTEST_TEST_COMMAND_DBUS})

set (TEST_COMMAND
     ${GTEST_TEST_COMMAND}
  && ${GTEST_TEST_COMMAND_SLOW}
  && ${TEST_COMMAND_XLESS})

set (TEST_COMMAND_HEADLESS
     export NUX_FALLBACK_TEXTURE=TRUE
  && ${DUMMY_XORG_TEST_RUNNER} ${GTEST_TEST_COMMAND}
  && ${DUMMY_XORG_TEST_RUNNER} ${GTEST_TEST_COMMAND_SLOW}
  && ${TEST_COMMAND_XLESS})

if (GTEST_ROOT_DIR)
  if (ENABLE_X_SUPPORT)
    add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-gtest test-gtest-slow test-gtest-xless test-gtest-dbus test-gestures)
    add_custom_target (check-headless COMMAND ${TEST_COMMAND_HEADLESS} DEPENDS test-gtest test-gtest-slow test-gtest-xless test-gtest-dbus test-gestures)
    add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest test-gtest-xless)
  else ()
    add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-gtest-xless test-gtest-dbus)
    add_custom_target (check-headless COMMAND ${TEST_COMMAND_XLESS} DEPENDS test-gtest-xless test-gtest-dbus)
    add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest-xless)
  endif ()
  add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
endif (GTEST_ROOT_DIR)
# make target to allow devs to run "make autopilot" from build dir:
set (AUTOPILOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/autopilot")
# Rules to install autopilot files and executable script:
install(CODE "execute_process(COMMAND python setup.py install --prefix ${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${AUTOPILOTDIR})")
add_custom_target (autopilot COMMAND cd ${AUTOPILOTDIR} && make check)
