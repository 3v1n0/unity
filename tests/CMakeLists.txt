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
set (TEST_DEPS "${UNITY_PLUGIN_DEPS}")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS})

set (TESTDATADIR "${CMAKE_CURRENT_SOURCE_DIR}/data")

set (CFLAGS
     ${TEST_UNIT_DEPS_CFLAGS}
     ${TEST_UNIT_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
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
                unit/TestFavoriteStoreGSettings.cpp
                unit/TestPlaceFactoryFile.cpp
                unit/TestQuicklistMenuitems.cpp
                unit/TestMain.cpp
                unit/TestUBus.cpp
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                ${UNITY_SRC}/FavoriteStore.cpp
                ${UNITY_SRC}/FavoriteStore.h
                ${UNITY_SRC}/FavoriteStoreGSettings.cpp
                ${UNITY_SRC}/FavoriteStoreGSettings.h
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

set (PLACES_COMMON_SOURCE ${UNITY_SRC}/ubus-server.cpp
                          ${UNITY_SRC}/ubus-server.h
                          ${UNITY_SRC}/IconLoader.cpp
                          ${UNITY_SRC}/IconLoader.h
                          ${UNITY_SRC}/IconTexture.cpp
                          ${UNITY_SRC}/IconTexture.h
                          ${UNITY_SRC}/Introspectable.cpp
                          ${UNITY_SRC}/Introspectable.h
                          ${UNITY_SRC}/PlacesSettings.cpp
                          ${UNITY_SRC}/PlacesSettings.h
                          ${UNITY_SRC}/PlacesStyle.cpp
                          ${UNITY_SRC}/PlacesStyle.h
                          ${UNITY_SRC}/PlacesTile.cpp
                          ${UNITY_SRC}/PlacesTile.h
                          ${UNITY_SRC}/PlacesHorizontalTile.cpp
                          ${UNITY_SRC}/PlacesHorizontalTile.h
                          ${UNITY_SRC}/PlacesSimpleTile.cpp
                          ${UNITY_SRC}/PlacesSimpleTile.h
                          ${UNITY_SRC}/TextureCache.h
                          ${UNITY_SRC}/TextureCache.cpp
                          ${UNITY_SRC}/Timer.cpp
                          ${UNITY_SRC}/Timer.h
                          ${UNITY_SRC}/StaticCairoText.cpp
                          ${UNITY_SRC}/StaticCairoText.h
                          ${UNITY_SRC}/UBusMessages.h
                          )
add_executable (test-places
                TestPlaces.cpp
                ${PLACES_COMMON_SOURCE}
                ${UNITY_SRC}/PlacesEmptyView.cpp
                ${UNITY_SRC}/PlacesEmptyView.h
                ${UNITY_SRC}/PlacesHomeView.cpp
                ${UNITY_SRC}/PlacesHomeView.h
                ${UNITY_SRC}/PlacesSearchBar.cpp
                ${UNITY_SRC}/PlacesSearchBar.h
                ${UNITY_SRC}/PlacesSearchBarSpinner.cpp
                ${UNITY_SRC}/PlacesSearchBarSpinner.h
                ${UNITY_SRC}/PlacesResultsController.cpp
                ${UNITY_SRC}/PlacesResultsController.h
                ${UNITY_SRC}/PlacesResultsView.h
                ${UNITY_SRC}/PlacesResultsView.cpp
                ${UNITY_SRC}/PlacesGroupController.cpp
                ${UNITY_SRC}/PlacesResultsController.h
                ${UNITY_SRC}/PlacesGroup.cpp
                ${UNITY_SRC}/PlacesGroup.h
                ${UNITY_SRC}/PlacesView.cpp
                ${UNITY_SRC}/PlacesView.h
                ${UNITY_SRC}/PlacesVScrollBar.cpp
                ${UNITY_SRC}/PlacesVScrollBar.h
                ${UNITY_SRC}/PlaceEntryHome.cpp
                ${UNITY_SRC}/PlaceEntryRemote.h
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
                )
add_dependencies (test-places unity-core-${UNITY_API_VERSION})

add_executable (test-places-tiles
                TestPlacesTiles.cpp
                ${PLACES_COMMON_SOURCE}
                )
add_dependencies (test-places-tiles unity-core-${UNITY_API_VERSION})

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

add_executable (test-places-group
                TestPlacesGroup.cpp
                ${PLACES_COMMON_SOURCE}
                ${UNITY_SRC}/PlacesGroup.cpp
                ${UNITY_SRC}/PlacesGroup.h
                )
add_dependencies (test-places-group unity-core-${UNITY_API_VERSION})

add_executable (test-places-results
                TestPlacesResults.cpp
                ${PLACES_COMMON_SOURCE}
                ${UNITY_SRC}/PlacesResultsController.cpp
                ${UNITY_SRC}/PlacesResultsController.h
                ${UNITY_SRC}/PlacesResultsView.h
                ${UNITY_SRC}/PlacesResultsView.cpp
                ${UNITY_SRC}/PlacesGroupController.cpp
                ${UNITY_SRC}/PlacesGroupController.h
                ${UNITY_SRC}/PlacesGroup.cpp
                ${UNITY_SRC}/PlacesGroup.h
                ${UNITY_SRC}/PlacesVScrollBar.cpp
                ${UNITY_SRC}/PlacesVScrollBar.h
                )
add_dependencies (test-places-results unity-core-${UNITY_API_VERSION})

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

add_executable (test-places-backend
                TestPlacesBackend.cpp
                ${UNITY_SRC}/PlaceEntryRemote.cpp
                ${UNITY_SRC}/PlaceEntryRemote.h
                ${UNITY_SRC}/PlaceEntry.h
                ${UNITY_SRC}/PlaceFactoryFile.cpp
                ${UNITY_SRC}/PlaceFactoryFile.h
                ${UNITY_SRC}/PlaceFactory.h
                ${UNITY_SRC}/PlaceFactory.cpp
                ${UNITY_SRC}/PlaceRemote.cpp
                ${UNITY_SRC}/PlaceRemote.h
                ${UNITY_SRC}/Place.h
                ${UNITY_SRC}/ubus-server.cpp
                ${UNITY_SRC}/ubus-server.h
                ${UNITY_SRC}/UBusMessages.h
                )
add_dependencies (test-places-backend unity-core-${UNITY_API_VERSION})

add_executable (test-filesystem-lenses
                TestFilesystemLoader.cpp) 
add_dependencies (test-filesystem-lenses unity-core-${UNITY_API_VERSION})

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
  add_executable(test-gtest
                 test_filesystem_lenses.cpp
                 test_glib_signals.cpp
                 test_glib_signals_utils.cpp
                 test_glib_signals_utils.h
                 ${CMAKE_CURRENT_BINARY_DIR}/test_glib_signals_utils_marshal.cpp
                 test_indicator_entry.cpp
                 test_main.cpp
                 test_timer.cpp
                 ${UNITY_SRC}/Timer.h
                 ${UNITY_SRC}/Timer.cpp
                 ${UNITY_SRC}/AbstractLauncherIcon.h
                 ${UNITY_SRC}/SwitcherModel.cpp
                 ${UNITY_SRC}/SwitcherModel.h
                 ${UNITY_SRC}/MockLauncherIcon.h
                 )
  target_link_libraries(test-gtest ${GTEST_BOTH_LIBRARIES})
  add_test(UnityGTests test-gtest)
  add_dependencies (test-gtest unity-core-${UNITY_API_VERSION})
  add_custom_target (gcheck COMMAND  ./test-gtest DEPENDS test-gtest)
endif (${GTEST_FOUND} STREQUAL TRUE)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (TEST_COMMAND gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit && ./test-gtest)
add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})

