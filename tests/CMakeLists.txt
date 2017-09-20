set(UNITY_SRC ../plugins/unityshell/src)

#
# Data
#
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR}/tests)

#
# Google Tests
#

# Check for google test and build it locally
set (GTEST_PREFIX "/usr/src/googletest")

if (EXISTS "${GTEST_PREFIX}")
  # As of version 1.8.0
  set (GMOCK_SOURCE_DIR "${GTEST_PREFIX}/googlemock" CACHE PATH "gmock source directory")
  set (GMOCK_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/include" CACHE PATH "gmock source include directory")
  set (GTEST_INCLUDE_DIR "${GTEST_PREFIX}/googletest/include" CACHE PATH "gtest source include directory")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
else()
  set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
  set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
  set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")
endif()

if (GTEST_INCLUDE_DIR AND GMOCK_INCLUDE_DIR)
  include_directories (${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
  add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")
elseif (GTEST_INCLUDE_DIR AND NOT GMOCK_INCLUDE_DIR)
  include_directories (${GTEST_INCLUDE_DIR})
endif(GTEST_INCLUDE_DIR AND GMOCK_INCLUDE_DIR)

if (GMOCK_SOURCE_DIR)
  set(GMOCK_LIB gmock)
  set(GMOCK_MAIN_LIB gmock_main)
endif (GMOCK_SOURCE_DIR)

#
# Unit tests
#
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};unity>=4.99.0;xtst")
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
include_directories (${CMAKE_SOURCE_DIR}/decorations)
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

set (UNITY_TEST_XLESS_BINARIES "")
set (UNITY_TEST_XLESS_TARGETS "")

set (UNITY_TEST_BINARIES "")
set (UNITY_TEST_TARGETS "")
set (UNITY_TEST_TARGETS_HEADLESS "")

set (UNITY_TEST_SLOW_BINARIES "")
set (UNITY_TEST_SLOW_TARGETS "")
set (UNITY_TEST_SLOW_TARGETS_HEADLESS "")

set (UNITY_TEST_DBUS_BINARIES "")
set (UNITY_TEST_DBUS_TARGETS "")
set (UNITY_TEST_DBUS_TARGETS_HEADLESS "")

set (DUMMY_XORG_TEST_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/dummy-xorg-test-runner.sh)
find_program (DBUS_RUN_SESSION dbus-run-session)

function (to_camel_case input output)
  string (REGEX MATCHALL "(^|[-_])([^-_]+)" matches ${input})

  foreach (match ${matches})
    string (REGEX REPLACE "^[-_]" "" match ${match})
    string (SUBSTRING ${match} 0 1 head)
    string (SUBSTRING ${match} 1 -1 tail)
    string (TOUPPER ${head} head)
    string (TOLOWER ${tail} tail)
    string (CONCAT camel ${camel} ${head} ${tail})
  endforeach ()

  set (${output} ${camel} PARENT_SCOPE)
endfunction ()

function (ensure_screaming_case input output)
  set (screaming)
  string (REPLACE "-" "_" screaming ${input})
  string (TOUPPER ${screaming} screaming)

  set (${output} ${screaming} PARENT_SCOPE)
endfunction ()

function (add_gtest basename)
  set(options "")
  set(oneValueArgs NAMESPACE)
  set(multiValueArgs LIBS EXTRA_SOURCES)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set (test_binary test-${basename})
  string (REPLACE "-" "_" test_source ${test_binary})

  to_camel_case (${ARGS_NAMESPACE} namespace_camel)
  to_camel_case (${basename} basename_camel)
  ensure_screaming_case (${ARGS_NAMESPACE} VAR_PREFIX)

  add_executable (${test_binary} ${test_source}.cpp ${ARGS_EXTRA_SOURCES})
  target_link_libraries (${test_binary} ${ARGS_LIBS})

  add_test ("${namespace_camel}${basename_camel}" ${test_binary})
  set (${VAR_PREFIX}_BINARIES ${${VAR_PREFIX}_BINARIES} ${test_binary} PARENT_SCOPE)

  set (run_target run-${test_binary})
  list (FIND ${VAR_PREFIX}_TARGETS ${run_target} _target_index)
  if (${_target_index} EQUAL -1)
    set (test_result ${CMAKE_CURRENT_BINARY_DIR}/${test_binary}.xml)
    add_custom_target (${run_target}
                      COMMAND ./${test_binary} --gtest_output=xml:${test_result}
                      DEPENDS ${test_binary})
    add_custom_target (clean-${test_binary}-result COMMAND rm -f ${test_result})
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} ${run_target} PARENT_SCOPE)
  endif ()

  set (run_target run-${test_binary}-headless)
  list (FIND ${VAR_PREFIX}_TARGETS ${run_target} _target_index)
  if (${_target_index} EQUAL -1)
    set (test_result ${CMAKE_CURRENT_BINARY_DIR}/${test_binary}-headless.xml)
    add_custom_target (${run_target}
                      COMMAND env NUX_FALLBACK_TEXTURE=TRUE
                              ${DUMMY_XORG_TEST_RUNNER}
                              ${DBUS_RUN_SESSION}
                              ./${test_binary} --gtest_output=xml:${test_result}
                      DEPENDS ${test_binary})
    add_custom_target (clean-${test_binary}-headless-result COMMAND rm -f ${test_result})
    set (${VAR_PREFIX}_TARGETS_HEADLESS ${${VAR_PREFIX}_TARGETS_HEADLESS} ${run_target} PARENT_SCOPE)
  endif ()

  # add_custom_command (OUTPUT ${test_result} COMMAND
                      # ./${test_binary} --gtest_output=xml:${test_result})
  # set (GTEST_RESULTS ${GTEST_RESULTS} ${test_result} PARENT_SCOPE)

  # add_custom_command (OUTPUT ${test_result}
                      # COMMAND env NUX_FALLBACK_TEXTURE=TRUE ${DUMMY_XORG_TEST_RUNNER}
                      # ./${test_binary} --gtest_output=xml:${test_result})
  # set (GTEST_RESULTS_HEADLESS ${GTEST_RESULTS_HEADLESS} ${test_result} PARENT_SCOPE)
endfunction ()

if (GMOCK_LIB AND
    GMOCK_MAIN_LIB)

  add_library (test-main-libs STATIC
               test_main.cpp
               logger_helper.cpp
              )
  # Build plain C files separately so they don't try to include the
  # C++ pch.
  add_library (test-libs-c STATIC
               bamf-mock-application.c
               bamf-mock-window.c
               gmockmount.c
               gmockvolume.c
               ${CMAKE_SOURCE_DIR}/services/panel-service.c
              )

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

  # The actual test executable (xless) - do not put anything that requires X in here

  add_library (test-main-xless-libs STATIC
               test_main_xless.cpp
               logger_helper.cpp
              )

  function (add_unity_test_xless basename)
    set (VAR_PREFIX "UNITY_TEST_XLESS")
    set (UNITY_TEST_XLESS_LIBS
         gtest
         test-main-xless-libs
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
      set (UNITY_TEST_XLESS_LIBS
           ${UNITY_TEST_XLESS_LIBS}
           shortcuts-lib
           hud-lib
          )
    endif ()

    add_gtest (${basename} NAMESPACE ${VAR_PREFIX} LIBS "${${VAR_PREFIX}_LIBS}" ${ARGN})

    set (${VAR_PREFIX}_BINARIES ${${VAR_PREFIX}_BINARIES} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} PARENT_SCOPE)
  endfunction ()

  add_unity_test_xless (abstract-interface-generator)
  add_unity_test_xless (action-handle)
  add_unity_test_xless (animation-utils)
  add_unity_test_xless (connection-manager)
  add_unity_test_xless (delta-tracker)
  add_unity_test_xless (desktop-application-subject)
  add_unity_test_xless (desktop-utilities)
  add_unity_test_xless (em-converter)
  add_unity_test_xless (favorite-store)
  add_unity_test_xless (favorite-store-gsettings)
  add_unity_test_xless (favorite-store-private)
  add_unity_test_xless (glib-cancellable)
  add_unity_test_xless (glib-dbus-object)
  add_unity_test_xless (glib-object EXTRA_SOURCES test_glib_object_utils.cpp)
  add_unity_test_xless (glib-signals EXTRA_SOURCES test_glib_signals_utils.cpp)
  add_unity_test_xless (glib-source)
  add_unity_test_xless (glib-variant)
  add_unity_test_xless (grabhandle
                        EXTRA_SOURCES
                        ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle.cpp
                        ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-group.cpp
                        ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-impl-factory.cpp
                        ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-grab-handle-layout.cpp
                        ${CMAKE_SOURCE_DIR}/plugins/unity-mt-grab-handles/src/unity-mt-texture.cpp)
  add_unity_test_xless (gsettings-scopes)
  add_unity_test_xless (indicator)
  add_unity_test_xless (indicator-appmenu)
  add_unity_test_xless (indicator-entry)
  add_unity_test_xless (indicators)
  add_unity_test_xless (introspection-data)
  add_unity_test_xless (launcher-entry-remote)
  add_unity_test_xless (launcher-options)
  add_unity_test_xless (layout-system)
  add_unity_test_xless (model-iterator)
  add_unity_test_xless (previews)
  add_unity_test_xless (raw-pixel)
  add_unity_test_xless (scope-data)
  add_unity_test_xless (time-util)
  add_unity_test_xless (ubus)
  add_unity_test_xless (unityshell-private EXTRA_SOURCES ${UNITY_SRC}/UnityshellPrivate.cpp)
  add_unity_test_xless (volume-imp)

  if (ENABLE_X_SUPPORT)
    add_unity_test_xless (hud-private)
    add_unity_test_xless (pointer-barrier)
    add_unity_test_xless (shortcut-model)
    add_unity_test_xless (shortcut-private)
  endif ()

  # tests that require dbus, must not require X
  add_library (test-main-dbus-libs STATIC
               test_main_dbus.cpp
               logger_helper.cpp
              )

  function (add_unity_test_dbus basename)
    set (VAR_PREFIX "UNITY_TEST_DBUS")
    set (UNITY_TEST_DBUS_LIBS gtest unity-shared test-main-dbus-libs ${LIBS})

    set (test_binary test-${basename})
    add_custom_target (run-${test_binary}
                       COMMAND dbus-test-runner --max-wait=300 --task
                       ./test-gtest-service --task-name test-service
                       --task=./${test_binary} --task-name=${test_binary}
                       --wait-for=com.canonical.Unity.Test
                       --parameter=--gtest_output=xml:./${test_binary}.xml
                       DEPENDS ${test_binary} test-gtest-service)
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} run-${test_binary})

    add_gtest (${basename} NAMESPACE ${VAR_PREFIX} LIBS "${${VAR_PREFIX}_LIBS}" ${ARGN})

    set (${VAR_PREFIX}_BINARIES ${${VAR_PREFIX}_BINARIES} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} PARENT_SCOPE)
  endfunction ()

  # test-gtest-dbus
  add_unity_test_dbus (categories)
  add_unity_test_dbus (dbus-indicators)
  add_unity_test_dbus (filter)
  add_unity_test_dbus (glib-dbus-proxy)
  add_unity_test_dbus (hud)
  add_unity_test_dbus (main-dbus)
  add_unity_test_dbus (model)
  add_unity_test_dbus (ratings-filter)
  add_unity_test_dbus (results)
  add_unity_test_dbus (scope)
  add_unity_test_dbus (scope-filter)
  add_unity_test_dbus (scope-proxy)
  add_unity_test_dbus (tracks)

 # Tests that require X
if (ENABLE_X_SUPPORT)

  function (add_unity_test basename #[[extra_sources]])
    set (VAR_PREFIX UNITY_TEST)
    set (UNITY_TEST_LIBS
         test-main-libs
         test-libs-c
         ${LIBS}
         gtest
         gmock
         dash-lib
         decorations-lib
         hud-lib
         launcher-lib
         lockscreen-lib
         panel-lib
         pam
         previews-lib
         shortcuts-lib
         shutdown-lib
         switcher-lib
         unity-shared
         unity-shared-bamf
         unity-shared-standalone)

    add_gtest (${basename} NAMESPACE ${VAR_PREFIX} LIBS "${${VAR_PREFIX}_LIBS}" ${ARGN})

    set (${VAR_PREFIX}_BINARIES ${${VAR_PREFIX}_BINARIES} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS_HEADLESS ${${VAR_PREFIX}_TARGETS_HEADLESS} PARENT_SCOPE)
  endfunction ()

  add_unity_test (action-link)
  add_unity_test (application-launcher-icon EXTRA_SOURCES mock-application.cpp)
  add_unity_test (bamf-application EXTRA_SOURCES mock-application.cpp)
  add_unity_test (bfb-launcher-icon)
  add_unity_test (decorations-input-mixer)
  add_unity_test (decorations-widgets)
  add_unity_test (dashview)
  add_unity_test (dashview-impl)
  add_unity_test (dash-controller)
  add_unity_test (desktop-launcher-icon)
  add_unity_test (device-launcher-section)
  add_unity_test (error-preview)
  add_unity_test (edge-barrier-controller)
  add_unity_test (expo-launcher-icon)
  add_unity_test (file-manager-launcher-icon)
  add_unity_test (filter-widgets)
  add_unity_test (glib-dbus-server)
  add_unity_test (gnome-session-manager)
  add_unity_test (gtk-icon-info)
  add_unity_test (hud-button)
  add_unity_test (hud-controller)
  add_unity_test (hud-launcher-icon)
  add_unity_test (hud-view)
  add_unity_test (icon-loader)
  add_unity_test (im-text-entry)
  add_unity_test (keyboard-util)
  add_unity_test (launcher EXTRA_SOURCES mock-application.cpp)
  add_unity_test (launcher-controller EXTRA_SOURCES mock-application.cpp)
  add_unity_test (launcher-drag-window)
  add_unity_test (launcher-hide-machine)
  add_unity_test (launcher-hover-machine)
  add_unity_test (launcher-icon)
  add_unity_test (launcher-minimize-speed
                  EXTRA_SOURCES
                  ${CMAKE_SOURCE_DIR}/plugins/unityshell/src/WindowMinimizeSpeedController.cpp)
  add_unity_test (launcher-model)
  add_unity_test (launcher-tooltip)
  add_unity_test (lockscreen-controller)
  add_unity_test (panel-controller)
  add_unity_test (panel-indicators-view)
  add_unity_test (panel-indicator-entry-dropdown-view)
  add_unity_test (panel-menu-view)
  add_unity_test (panel-service)
  add_unity_test (panel-style)
  add_unity_test (panel-tray)
  add_unity_test (panel-view)
  add_unity_test (places-group)
  add_unity_test (preview-player)
  add_unity_test (previews-application)
  add_unity_test (previews-generic)
  add_unity_test (previews-movie)
  add_unity_test (previews-music)
  add_unity_test (previews-music-payment)
  add_unity_test (previews-payment)
  add_unity_test (previews-social)
  add_unity_test (quicklist-manager)
  add_unity_test (quicklist-menu-item)
  add_unity_test (quicklist-view)
  add_unity_test (result-renderer)
  add_unity_test (resultviewgrid)
  add_unity_test (scope-bar)
  add_unity_test (scope-view)
  add_unity_test (screensaver-dbus-manager)
  add_unity_test (searchbar)
  add_unity_test (session-button)
  add_unity_test (session-controller)
  add_unity_test (session-view)
  add_unity_test (shortcut-controller)
  add_unity_test (shortcut-modeller-compiz)
  add_unity_test (shortcut-view)
  add_unity_test (single-monitor-launcher-icon)
  add_unity_test (showdesktop-handler
                  EXTRA_SOURCES ${UNITY_SRC}/UnityShowdesktopHandler.cpp)
  add_unity_test (software-center-launcher-icon EXTRA_SOURCES mock-application.cpp)
  add_unity_test (spread-filter)
  add_unity_test (static-cairo-text)
  add_unity_test (switcher-controller
                  EXTRA_SOURCES test_switcher_controller_class.cpp)
  add_unity_test (switcher-model)
  add_unity_test (switcher-view)
  add_unity_test (systemd-wrapper)
  add_unity_test (tabiterator)
  add_unity_test (texture-cache)
  add_unity_test (text-input)
  add_unity_test (thumbnail-generator)
  add_unity_test (trash-launcher-icon EXTRA_SOURCES mock-application.cpp)
  add_unity_test (unity-settings)
  add_unity_test (unity-window-style)
  add_unity_test (unity-window-view)
  add_unity_test (upstart-wrapper)
  add_unity_test (user-authenticator-pam)
  add_unity_test (volume-launcher-icon)
  add_unity_test (window-buttons)
  add_unity_test (xdnd-manager-imp)
  add_unity_test (xdnd-start-stop-notifier-imp)

  #
  #  Slow tests
  #
  function (add_unity_test_slow basename)
    set (VAR_PREFIX "UNITY_TEST_SLOW")
    set (UNITY_TEST_SLOW_LIBS
         test-main-libs
         gtest
         gmock
         launcher-lib
         switcher-lib
         unity-shared
         unity-shared-standalone
      )

    add_gtest (${basename} NAMESPACE ${VAR_PREFIX} LIBS "${${VAR_PREFIX}_LIBS}" ${ARGN})

    set (${VAR_PREFIX}_BINARIES ${${VAR_PREFIX}_BINARIES} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS ${${VAR_PREFIX}_TARGETS} PARENT_SCOPE)
    set (${VAR_PREFIX}_TARGETS_HEADLESS ${${VAR_PREFIX}_TARGETS_HEADLESS} PARENT_SCOPE)
  endfunction ()

  add_unity_test_slow (switcher-controller-slow
                       EXTRA_SOURCES
                       test_switcher_controller_class.cpp
                       mock-application.cpp)
  add_unity_test_slow (tooltip-manager)

endif (ENABLE_X_SUPPORT)
endif (GMOCK_LIB AND
       GMOCK_MAIN_LIB)

if(ENABLE_X_SUPPORT)
  add_subdirectory (test-gestures)
endif()

#
# check target
#
if (GMOCK_SOURCE_DIR)
  add_custom_target (tests-xless DEPENDS ${UNITY_TEST_XLESS_BINARIES})
  add_custom_target (test-gtest-xless DEPENDS tests-xless)
  add_custom_target (check-xless DEPENDS ${UNITY_TEST_XLESS_TARGETS})

  add_custom_target (tests-dbus DEPENDS ${UNITY_TEST_DBUS_BINARIES})
  add_custom_target (test-gtest-dbus DEPENDS tests-dbus)
  add_custom_target (check-dbus DEPENDS ${UNITY_TEST_DBUS_TARGETS})

  add_custom_target (clean-tests-results COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/test-*.xml)

  if (ENABLE_X_SUPPORT)
    add_custom_target (test-gtest DEPENDS ${UNITY_TEST_BINARIES})
    add_custom_target (check-gtest DEPENDS ${UNITY_TEST_TARGETS})
    add_custom_target (check-gtest-headless DEPENDS ${UNITY_TEST_TARGETS_HEADLESS})

    add_custom_target (test-gtest-slow DEPENDS ${UNITY_TEST_SLOW_BINARIES})
    add_custom_target (check-gtest-slow DEPENDS ${UNITY_TEST_SLOW_TARGETS})
    add_custom_target (check-gtest-slow-headless DEPENDS ${UNITY_TEST_SLOW_TARGETS_HEADLESS})

    add_custom_target (tests DEPENDS
                       test-gtest
                       test-gtest-slow
                       test-gtest-xless
                       test-gtest-dbus
                       test-gestures
                      )

    add_custom_target (check DEPENDS
                       check-xless
                       check-gestures
                       check-gtest
                       check-gtest-slow
                       check-dbus
                      )

    add_custom_target (check-headless DEPENDS
                       check-xless
                       check-gestures
                       check-gtest-headless
                       check-gtest-slow-headless
                       check-dbus
                      )

    add_custom_target (gcheck DEPENDS check)
  else ()
    add_custom_target (tests DEPENDS
                       test-gtest-xless
                       test-gtest-dbus
                      )

    add_custom_target (check DEPENDS
                       check-xless
                       check-dbus)

    add_custom_target (check-headless DEPENDS check)
    add_custom_target (gcheck DEPENDS check)
  endif ()

endif (GMOCK_SOURCE_DIR)

# make target to allow devs to run "make autopilot" from build dir:
set (AUTOPILOTDIR "${CMAKE_CURRENT_SOURCE_DIR}/autopilot")
# Rules to install autopilot files and executable script:
install(CODE "execute_process(COMMAND python2.7 setup.py install --prefix ${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${AUTOPILOTDIR})")
add_custom_target (autopilot COMMAND cd ${AUTOPILOTDIR} && make check)
