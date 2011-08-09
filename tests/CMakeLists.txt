set(UNITY_SRC ../plugins/unityshell/src)

#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/update-manager.desktop)
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

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES} "-lunity-core-${UNITY_API_VERSION}")
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${CMAKE_BINARY_DIR}/UnityCore ${LIB_PATHS})

include_directories (. .. ../services ../UnityCore ${UNITY_SRC} ${CMAKE_BINARY_DIR})

# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../${UNITY_SRC} or ../../services in alphabetically after that
add_executable (test-unit
                unit/TestPlaceFactoryFile.cpp
                unit/TestQuicklistMenuitems.cpp
                unit/TestMain.cpp
                unit/TestUBus.cpp
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/PlaceEntryRemote.cpp
                ${UNITY_SRC}/PlaceEntryRemote.h
                ${UNITY_SRC}/PlaceEntry.h
                ${UNITY_SRC}/PlaceFactoryFile.cpp
                ${UNITY_SRC}/PlaceFactoryFile.h
                ${UNITY_SRC}/PlaceFactory.cpp
                ${UNITY_SRC}/PlaceFactory.h
                ${UNITY_SRC}/PlaceRemote.cpp
                ${UNITY_SRC}/PlaceRemote.h
                ${UNITY_SRC}/Place.h
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

#
# Standalone tests
#
add_executable (standalone-dash
                standalone_dash.cpp
                ${UNITY_SRC}/BGHash.cpp
                ${UNITY_SRC}/BGHash.h
                ${UNITY_SRC}/DashSearchBar.cpp
                ${UNITY_SRC}/DashSearchBar.h
                ${UNITY_SRC}/DashSearchBarSpinner.cpp
                ${UNITY_SRC}/DashSearchBarSpinner.h
                ${UNITY_SRC}/PlacesGroup.cpp
                ${UNITY_SRC}/PlacesGroup.h
                ${UNITY_SRC}/PlacesStyle.cpp
                ${UNITY_SRC}/PlacesStyle.h
                ${UNITY_SRC}/DashView.cpp
                ${UNITY_SRC}/DashView.h
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/IconLoader.h
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconTexture.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/LensView.cpp
                ${UNITY_SRC}/LensView.h
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/ResultView.cpp
                ${UNITY_SRC}/ResultViewGrid.cpp
                ${UNITY_SRC}/ResultRenderer.cpp
                ${UNITY_SRC}/ResultRendererTile.cpp
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
                )
add_dependencies (standalone-dash unity-core-${UNITY_API_VERSION})


#
# Please use "standalone-$foo" naming convention for standalone tests
# 
add_executable (test-panel
                TestPanel.cpp
                ${UNITY_SRC}/PanelStyle.cpp
                ${UNITY_SRC}/PanelStyle.h
                ${UNITY_SRC}/PanelView.cpp
                ${UNITY_SRC}/PanelView.h
                ${UNITY_SRC}/PanelIndicatorObjectView.cpp
                ${UNITY_SRC}/PanelIndicatorObjectView.h
                ${UNITY_SRC}/PanelIndicatorObjectEntryView.cpp
                ${UNITY_SRC}/PanelIndicatorObjectEntryView.h
                ${UNITY_SRC}/PanelTitlebarGrabAreaView.h
                ${UNITY_SRC}/PanelTitlebarGrabAreaView.cpp
                ${UNITY_SRC}/PanelTray.cpp
                ${UNITY_SRC}/PanelTray.h
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/Introspectable.h
                ${UNITY_SRC}/PanelHomeButton.cpp
                ${UNITY_SRC}/PanelHomeButton.h
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
add_dependencies (test-panel unity-core-${UNITY_API_VERSION})

add_executable (test-switcher
                TestSwitcher.cpp
                ${UNITY_SRC}/AbstractLauncherIcon.h
                ${UNITY_SRC}/AbstractIconRenderer.h
                ${UNITY_SRC}/SwitcherController.cpp
                ${UNITY_SRC}/SwitcherController.h
                ${UNITY_SRC}/SwitcherModel.cpp
                ${UNITY_SRC}/SwitcherModel.h
                ${UNITY_SRC}/SwitcherView.cpp
                ${UNITY_SRC}/SwitcherView.h
                ${UNITY_SRC}/IconRenderer.cpp
                ${UNITY_SRC}/IconRenderer.h
                ${UNITY_SRC}/MockLauncherIcon.h
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/StaticCairoText.h
                )
add_dependencies (test-switcher unity-core-${UNITY_API_VERSION})

add_executable (test-quicklist
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
add_dependencies (test-quicklist unity-core-${UNITY_API_VERSION})

add_executable (test-quicklist-visuals
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
add_dependencies (test-quicklist-visuals unity-core-${UNITY_API_VERSION})

add_executable (test-filters
                TestFilters.cpp
                ${UNITY_SRC}/FilterExpanderLabel.cpp
                ${UNITY_SRC}/FilterBasicButton.cpp
                ${UNITY_SRC}/FilterRatingsButton.cpp
                ${UNITY_SRC}/FilterRatingsWidget.cpp
                ${UNITY_SRC}/FilterGenreButton.cpp
                ${UNITY_SRC}/FilterGenreWidget.cpp
                )
add_dependencies(test-filters unity-core-${UNITY_API_VERSION})

add_executable (test-filter-bar
                TestFilterBar.cpp
                ${UNITY_SRC}/FilterExpanderLabel.cpp
                ${UNITY_SRC}/FilterFactory.cpp
                ${UNITY_SRC}/FilterBasicButton.cpp
                ${UNITY_SRC}/FilterRatingsButton.cpp
                ${UNITY_SRC}/FilterRatingsWidget.cpp
                ${UNITY_SRC}/FilterGenreButton.cpp
                ${UNITY_SRC}/FilterGenreWidget.cpp
                ${UNITY_SRC}/FilterBar.cpp
                )
add_dependencies(test-filter-bar unity-core-${UNITY_API_VERSION})

add_executable (test-preview-applicaiton
                TestPreviewApplications.cpp
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                )
add_dependencies(test-preview-applicaiton unity-core-${UNITY_API_VERSION})

add_executable (test-preview-generic
                TestPreviewGeneric.cpp
                ${UNITY_SRC}/PreviewGeneric.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                )
add_dependencies(test-preview-generic unity-core-${UNITY_API_VERSION})

add_executable (test-preview-music
                TestPreviewMusic.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/PreviewMusic.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                )
add_dependencies(test-preview-generic unity-core-${UNITY_API_VERSION})

add_executable (test-result-view
                TestResultView.cpp
                ${UNITY_SRC}/PreviewApplications.cpp
                ${UNITY_SRC}/PreviewBase.cpp
                ${UNITY_SRC}/PreviewBasicButton.cpp
                ${UNITY_SRC}/ResultView.cpp
                ${UNITY_SRC}/ResultViewGrid.cpp
                ${UNITY_SRC}/ResultRenderer.cpp
                ${UNITY_SRC}/ResultRendererTile.cpp
                ${UNITY_SRC}/IconTexture.cpp
                ${UNITY_SRC}/IconLoader.cpp
                ${UNITY_SRC}/Introspectable.cpp
                ${UNITY_SRC}/StaticCairoText.cpp
                ${UNITY_SRC}/TextureCache.cpp
                )
add_dependencies(test-result-view unity-core-${UNITY_API_VERSION})

add_executable (test-bg-hash
                TestBGHash.cpp
                ${UNITY_SRC}/BGHash.cpp
                ${UNITY_SRC}/BGHash.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
               )
add_dependencies (test-bg-hash unity-core-${UNITY_API_VERSION})

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

# The actual test executable
  add_executable(test-gtest
                 test_categories.cpp
                 test_filesystem_lenses.cpp
                 test_filter.cpp
                 test_glib_signals.cpp
                 test_glib_signals_utils.cpp
                 test_glib_signals_utils.h
                 ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
                 test_favorite_store_gsettings.cpp
                 test_indicator_entry.cpp
                 test_lens.cpp
                 test_main.cpp
                 test_model.cpp
                 test_utils.h
                 test_ratings_filter.cpp
                 test_results.cpp
                 test_timer.cpp
                 ${UNITY_SRC}/AbstractLauncherIcon.h
                 ${UNITY_SRC}/FavoriteStore.cpp
                 ${UNITY_SRC}/FavoriteStore.h
                 ${UNITY_SRC}/FavoriteStoreGSettings.cpp
                 ${UNITY_SRC}/FavoriteStoreGSettings.h
                 ${UNITY_SRC}/MockLauncherIcon.h
                 ${UNITY_SRC}/SwitcherModel.cpp
                 ${UNITY_SRC}/SwitcherModel.h
                 ${UNITY_SRC}/Timer.cpp
                 ${UNITY_SRC}/Timer.h
                 )
  target_link_libraries(test-gtest ${GTEST_BOTH_LIBRARIES})
  add_test(UnityGTests test-gtest)
  add_dependencies (test-gtest unity-core-${UNITY_API_VERSION} test-gtest-service)
endif (${GTEST_FOUND} STREQUAL TRUE)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (DBUS_TEST_COMMAND dbus-test-runner --task ./test-gtest-service --task ./test-gtest)

set (TEST_COMMAND
     gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit
  && ${DBUS_TEST_COMMAND})
add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
add_custom_target (gcheck COMMAND  ${DBUS_TEST_COMMAND} DEPENDS test-gtest)

