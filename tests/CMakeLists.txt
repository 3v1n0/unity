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
add_executable (test-unit
                unit/TestQuicklistMenuitems.cpp
                unit/TestMain.cpp
                unit/TestUBus.cpp
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                ${UNITY_SRC}/Introspectable.cpp
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
                ${UNITY_SRC}/
                )
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
find_package(GTest)
if (${GTEST_FOUND} STREQUAL TRUE)
  include_directories(${GTEST_INCLUDE_DIRS})

# The service that provides DBus services to test against
  add_executable(test-gtest-service
                 test_service_lens.c
                 test_service_lens.h
                 test_service_main.c
                 test_service_model.c
                 test_service_model.h)
  add_dependencies (test-gtest-service unity-core-${UNITY_API_VERSION})

# The actual test executable (xless) - do not put anything that requires X in here
  add_executable(test-gtest-xless
                 test_glib_signals.cpp
                 test_glib_signals_utils.cpp
                 test_glib_signals_utils.h
                 ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
                 test_favorite_store_gsettings.cpp
                 test_main_xless.cpp
                 ${UNITY_SRC}/AbstractLauncherIcon.h
                 ${UNITY_SRC}/FavoriteStore.cpp
                 ${UNITY_SRC}/FavoriteStore.h
                 ${UNITY_SRC}/FavoriteStoreGSettings.cpp
                 ${UNITY_SRC}/FavoriteStoreGSettings.h
                 ${UNITY_SRC}/MockLauncherIcon.h
                 ${UNITY_SRC}/SwitcherModel.cpp
                 ${UNITY_SRC}/SwitcherModel.h
                 ${UNITY_SRC}/TextureCache.cpp
                 ${UNITY_SRC}/TextureCache.h
                 ${UNITY_SRC}/Timer.cpp
                 ${UNITY_SRC}/Timer.h
                 ${UNITY_SRC}/WindowManager.cpp
                 ${UNITY_SRC}/WindowManager.h
                 )
  target_link_libraries(test-gtest-xless ${GTEST_BOTH_LIBRARIES})
  add_test(UnityGTestXless test-gtest-xless)
  add_dependencies(test-gtest-xless unity-core-${UNITY_API_VERSION})

# tests that require dbus, must not require X
  add_executable(test-gtest-dbus
                 test_categories.cpp
                 test_filesystem_lenses.cpp
                 test_filter.cpp
                 test_indicator_entry.cpp
                 test_lens.cpp
                 test_main_dbus.cpp
                 test_model.cpp
                 test_utils.h
                 test_ratings_filter.cpp
                 test_results.cpp
                 )
  target_link_libraries(test-gtest-dbus ${GTEST_BOTH_LIBRARIES})
  add_test(UnityGTestDBus test-gtest-dbus)
  add_dependencies(test-gtest-dbus unity-core-${UNITY_API_VERSION} test-gtest-service)

# Tests that require X
  add_executable(test-gtest
                 test_texture_cache.cpp
                 test_main.cpp
                 ${UNITY_SRC}/TextureCache.cpp
                 ${UNITY_SRC}/TextureCache.h
                )
  target_link_libraries(test-gtest ${GTEST_BOTH_LIBRARIES})
  add_test(UnityGTest test-gtest)
  add_dependencies(test-gtest unity-core-${UNITY_API_VERSION})

endif (${GTEST_FOUND} STREQUAL TRUE)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (GTEST_TEST_COMMAND ./test-gtest)
set (GTEST_TEST_COMMAND_XLESS ./test-gtest-xless)
set (GTEST_TEST_COMMAND_DBUS dbus-test-runner --task ./test-gtest-service --task ./test-gtest-dbus)

set (TEST_COMMAND
     gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit
  && ${GTEST_TEST_COMMAND}
  && ${GTEST_TEST_COMMAND_XLESS}
  && ${GTEST_TEST_COMMAND_DBUS})

set (TEST_COMMAND_HEADLESS
     ${GTEST_TEST_COMMAND_XLESS}
  && ${GTEST_TEST_COMMAND_DBUS})

add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest test-gtest-xless test-gtest-dbus)
add_custom_target (check-headless COMMAND ${TEST_COMMAND_HEADLESS} DEPENDS test-gtest-xless test-gtest-dbus)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest test-gtest-xless)

