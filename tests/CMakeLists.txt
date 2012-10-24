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
                unit/TestStaticCairoText.cpp
                ../services/panel-service.c
                ${CMAKE_CURRENT_BINARY_DIR}/panel-marshal.c
                )
target_link_libraries (test-unit unity-shared ${LIBS})
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
                 test_service_model.h
                 test_service_panel.c
                 test_service_panel.c)
  target_link_libraries(test-gtest-service unity-shared ${LIBS})
  add_dependencies (test-gtest-service unity-core-${UNITY_API_VERSION} gtest)


# The actual test executable (xless) - do not put anything that requires X in here
  set (GTEST_XLESS_SOURCES
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
       test_indicator.cpp
       test_indicator_appmenu.cpp
       test_indicator_entry.cpp
       test_indicators.cpp
       test_introspection.cpp
       test_favorite_store.cpp
       test_favorite_store_gsettings.cpp
       test_favorite_store_private.cpp
       test_home_lens.cpp
       test_launcher_entry_remote.cpp
       test_model_iterator.cpp
       test_previews.cpp
       test_ubus.cpp
       test_unityshell_private.cpp
       ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
       ${UNITY_SRC}/UnityshellPrivate.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-group.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-impl-factory.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-layout.cpp
       ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-texture.cpp
)

if (ENABLE_X_SUPPORT)
  set (GTEST_XLESS_SOURCES
    test_hud_private.cpp
    test_pointer_barrier.cpp
    test_shortcut_model.cpp
    test_shortcut_private.cpp
    ${LAUNCHER_SOURCES}
    )
endif ()

  add_executable(test-gtest-xless ${GTEST_XLESS_SOURCES})


  target_link_libraries(test-gtest-xless gtest unity-shared ${GMOCK_LIB} ${GMOCK_MAIN_LIB} ${LIBS} launcher-lib)
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
                 test_dbus_indicators.cpp
                )
  target_link_libraries(test-gtest-dbus gtest unity-shared ${LIBS})
  add_test(UnityGTestDBus test-gtest-dbus)
  add_dependencies(test-gtest-dbus unity-core-${UNITY_API_VERSION} test-gtest-service gtest)

if (ENABLE_X_SUPPORT)
# Tests that require X
  add_executable(test-gtest
                 test_main.cpp
                 test_application_launcher_icon.cpp
                 test_bfb_launcher_icon.cpp
                 test_dashview_impl.cpp
                 test_desktop_launcher_icon.cpp
                 test_edge_barrier_controller.cpp
                 test_launcher.cpp
                 test_device_launcher_section.cpp
                 test_hud_button.cpp
                 test_hud_controller.cpp
                 test_hud_launcher_icon.cpp
                 test_hud_view.cpp
                 test_icon_loader.cpp
                 test_im_text_entry.cpp
                 test_launcher_controller.cpp
                 test_launcher_drag_window.cpp
                 test_launcher_icon.cpp
                 test_keyboard_util.cpp
                 test_panel_style.cpp
                 test_panel_menu_view.cpp
                 test_places_group.cpp
                 test_previews_application.cpp
                 test_previews_generic.cpp
                 test_previews_movie.cpp
                 test_previews_music.cpp
                 test_previews_social.cpp
                 test_quicklist_menu_item.cpp
                 test_quicklist_view.cpp
                 test_resultviewgrid.cpp
                 test_shortcut_controller.cpp
                 test_single_monitor_launcher_icon.cpp
                 test_expo_launcher_icon.cpp
                 test_showdesktop_handler.cpp
                 test_switcher_controller.cpp
                 test_switcher_model.cpp
                 test_texture_cache.cpp
                 test_thumbnail_generator.cpp
                 test_trash_launcher_icon.cpp
                 test_launcher_minimize_speed.cpp
                 test_unity_settings.cpp
                 test_volume_imp.cpp
                 test_volume_launcher_icon.cpp
                 bamf-mock-application.c
                 gmockmount.c
                 gmockvolume.c
                 ${UNITY_SRC}/UnityShowdesktopHandler.cpp
                 ${CMAKE_SOURCE_DIR}/plugins/unityshell/src/WindowMinimizeSpeedController.cpp
                )
  target_link_libraries(test-gtest gtest gmock unity-shared ${LIBS} launcher-lib unity-shared-standalone shortcuts-lib previews-lib hud-lib switcher-lib dash-lib panel-lib)
  add_test(UnityGTest test-gtest)
  add_dependencies(test-gtest unity-core-${UNITY_API_VERSION} gtest gmock)
endif (ENABLE_X_SUPPORT)
endif (GTEST_SRC_DIR AND
       GMOCK_LIB AND
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
if (ENABLE_X_SUPPORT)
  set (GTEST_TEST_COMMAND ./test-gtest)
  set (GTEST_TEST_COMMAND_GESTURES ./test-gestures/test-gestures)
endif (ENABLE_X_SUPPORT)
set (GTEST_TEST_COMMAND_XLESS ./test-gtest-xless)
set (GTEST_TEST_COMMAND_DBUS dbus-test-runner --task ./test-gtest-service --task ./test-gtest-dbus)

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
  if (ENABLE_X_SUPPORT)
    add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest test-gtest-xless test-gtest-dbus test-gestures)
    add_custom_target (check-headless COMMAND ${TEST_COMMAND_HEADLESS} DEPENDS test-gtest-xless test-gtest-dbus test-gestures)
    add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest test-gtest-xless)
  else ()
    add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest-xless test-gtest-dbus)
    add_custom_target (check-headless COMMAND ${TEST_COMMAND_HEADLESS} DEPENDS test-gtest-xless test-gtest-dbus)
    add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest-xless)
  endif ()
  add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
else (GTEST_SRC_DIR)
  add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit)
endif (GTEST_SRC_DIR)
# make target to allow devs to run "make autopilot" from build dir:
set (AUTOPILOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/autopilot")
# Rules to install autopilot files and executable script:
install(CODE "execute_process(COMMAND python setup.py install --prefix ${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${AUTOPILOTDIR})")
add_custom_target (autopilot COMMAND cd ${AUTOPILOTDIR} && make check)
