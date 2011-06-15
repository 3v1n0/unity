#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/update-manager.desktop)
#
# Unit tests
#
find_package (PkgConfig)
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};indicator")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS} indicator)

set (CFLAGS
     ${TEST_UNIT_DEPS_CFLAGS}
     ${TEST_UNIT_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DTESTDATADIR=${TESTDATADIR}"
     "-DGETTEXT_PACKAGE=\"unity\""
     "-DINDICATORDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-DINDICATORICONDIR=\"${CMAKE_BINARY_DIR}/tests\""
     )
add_definitions (${CFLAGS})

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES})
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${LIB_PATHS})

include_directories (. .. ../services ${CMAKE_BINARY_DIR})

find_program(GLIB_GENMARSHAL glib-genmarshal)
add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/services/panel-marshal.c
  COMMAND ${GLIB_GENMARSHAL} ARGS ${CMAKE_SOURCE_DIR}/services/panel-marshal.list --body --prefix=panel_marshal > ${CMAKE_SOURCE_DIR}/services/panel-marshal.c
  COMMAND ${GLIB_GENMARSHAL} ARGS ${CMAKE_SOURCE_DIR}/services/panel-marshal.list --header --prefix=panel_marshal > ${CMAKE_SOURCE_DIR}/services/panel-marshal.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ../services/panel-marshal.list
  COMMENT "Generating marshallers")

# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../../src or ../../services in alphabetically after that
add_executable (test-unit
                unit/TestFavoriteStoreGSettings.cpp
                unit/TestPanelService.cpp
                unit/TestPlaceFactoryFile.cpp
                unit/TestQuicklistMenuitems.cpp
                unit/TestMain.cpp
                unit/TestUBus.cpp
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                ../services/panel-service.c
                ../services/panel-service.h
                ${CMAKE_SOURCE_DIR}/services/panel-marshal.c
                ../src/FavoriteStore.cpp
                ../src/FavoriteStore.h
                ../src/FavoriteStoreGSettings.cpp
                ../src/FavoriteStoreGSettings.h
                ../src/GLibWrapper.h
                ../src/GLibWrapper-inl.h
                ../src/GLibWrapper.cpp
                ../src/Introspectable.cpp
                ../src/PlaceEntryRemote.cpp
                ../src/PlaceEntryRemote.h
                ../src/PlaceEntry.h
                ../src/PlaceFactoryFile.cpp
                ../src/PlaceFactoryFile.h
                ../src/PlaceFactory.cpp
                ../src/PlaceFactory.h
                ../src/PlaceRemote.cpp
                ../src/PlaceRemote.h
                ../src/Place.h
                ../src/QuicklistMenuItem.cpp
                ../src/QuicklistMenuItem.h
                ../src/QuicklistMenuItemCheckmark.cpp
                ../src/QuicklistMenuItemCheckmark.h
                ../src/QuicklistMenuItemLabel.cpp
                ../src/QuicklistMenuItemLabel.h
                ../src/QuicklistMenuItemRadio.cpp
                ../src/QuicklistMenuItemRadio.h
                ../src/QuicklistMenuItemSeparator.cpp
                ../src/QuicklistMenuItemSeparator.h
                ../src/QuicklistView.cpp
                ../src/QuicklistView.h
                ../src/Variant.cpp
                ../src/Variant.h
                ../src/
                )

add_executable (test-panel
                TestPanel.cpp
                ../src/DBusIndicators.cpp
                ../src/DBusIndicators.h
                ../src/GLibWrapper.h
                ../src/GLibWrapper-inl.h
                ../src/GLibWrapper.cpp
                ../src/PanelStyle.cpp
                ../src/PanelStyle.h
                ../src/PanelView.cpp
                ../src/PanelView.h
                ../src/PanelIndicatorObjectView.cpp
                ../src/PanelIndicatorObjectView.h
                ../src/PanelIndicatorObjectEntryView.cpp
                ../src/PanelIndicatorObjectEntryView.h
            	../src/PanelTitlebarGrabAreaView.h
            	../src/PanelTitlebarGrabAreaView.cpp
                ../src/PanelTray.cpp
                ../src/PanelTray.h
                ../src/Indicators.cpp
                ../src/Indicators.h
                ../src/Indicator.cpp
                ../src/Indicator.h
                ../src/IndicatorEntry.cpp
                ../src/IndicatorEntry.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/PanelHomeButton.cpp
                ../src/PanelHomeButton.h
                ../src/PanelMenuView.cpp
                ../src/PanelMenuView.h
                ../src/Timer.cpp
                ../src/Timer.h
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/WindowButtons.cpp
                ../src/WindowButtons.h
                ../src/WindowManager.cpp
                ../src/WindowManager.h
                ../src/UScreen.cpp
                ../src/UScreen.h
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                ../src/Variant.cpp
                ../src/Variant.h
                )

set (PLACES_COMMON_SOURCE ../src/ubus-server.cpp
                          ../src/ubus-server.h
                          ../src/IconLoader.cpp
                          ../src/IconLoader.h
                          ../src/IconTexture.cpp
                          ../src/IconTexture.h
                          ../src/Introspectable.cpp
                          ../src/Introspectable.h
                          ../src/PlacesSettings.cpp
                          ../src/PlacesSettings.h
                          ../src/PlacesStyle.cpp
                          ../src/PlacesStyle.h
                          ../src/PlacesTile.cpp
                          ../src/PlacesTile.h
                          ../src/PlacesHorizontalTile.cpp
                          ../src/PlacesHorizontalTile.h
                          ../src/PlacesSimpleTile.cpp
                          ../src/PlacesSimpleTile.h
                          ../src/TextureCache.h
                          ../src/TextureCache.cpp
                          ../src/Timer.cpp
                          ../src/Timer.h
                          ../src/StaticCairoText.cpp
                          ../src/StaticCairoText.h
                          ../src/UBusMessages.h
                          ../src/Variant.cpp
                          ../src/Variant.h
                          )
add_executable (test-places
                TestPlaces.cpp
                ${PLACES_COMMON_SOURCE}
                ../src/PlacesEmptyView.cpp
                ../src/PlacesEmptyView.h
                ../src/PlacesHomeView.cpp
                ../src/PlacesHomeView.h
                ../src/PlacesSearchBar.cpp
                ../src/PlacesSearchBar.h
                ../src/PlacesSearchBarSpinner.cpp
                ../src/PlacesSearchBarSpinner.h
                ../src/PlacesResultsController.cpp
                ../src/PlacesResultsController.h
                ../src/PlacesResultsView.h
                ../src/PlacesResultsView.cpp
                ../src/PlacesGroupController.cpp
                ../src/PlacesResultsController.h
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                ../src/PlacesView.cpp
                ../src/PlacesView.h
                ../src/PlacesVScrollBar.cpp
                ../src/PlacesVScrollBar.h
                ../src/PlaceEntryHome.cpp
                ../src/PlaceEntryRemote.h
                ../src/PlaceEntryRemote.cpp
                ../src/PlaceEntryRemote.h
                ../src/PlaceEntry.h
                ../src/PlaceFactoryFile.cpp
                ../src/PlaceFactoryFile.h
                ../src/PlaceFactory.cpp
                ../src/PlaceFactory.h
                ../src/PlaceRemote.cpp
                ../src/PlaceRemote.h
                ../src/Place.h
                )

add_executable (test-places-tiles
                TestPlacesTiles.cpp
                ${PLACES_COMMON_SOURCE}
                )


add_executable (test-places-group
                TestPlacesGroup.cpp
                ${PLACES_COMMON_SOURCE}
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                )

add_executable (test-places-results
                TestPlacesResults.cpp
                ${PLACES_COMMON_SOURCE}
                ../src/PlacesResultsController.cpp
                ../src/PlacesResultsController.h
                ../src/PlacesResultsView.h
                ../src/PlacesResultsView.cpp
                ../src/PlacesGroupController.cpp
                ../src/PlacesGroupController.h
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                ../src/PlacesVScrollBar.cpp
                ../src/PlacesVScrollBar.h
                )

add_executable (test-quicklist
                ui/TestQuicklist.cpp
                ui/EventFaker.cpp
                ui/EventFaker.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/QuicklistMenuItem.cpp
                ../src/QuicklistMenuItem.h
                ../src/QuicklistMenuItemCheckmark.cpp
                ../src/QuicklistMenuItemCheckmark.h
                ../src/QuicklistMenuItemLabel.cpp
                ../src/QuicklistMenuItemLabel.h
                ../src/QuicklistMenuItemRadio.cpp
                ../src/QuicklistMenuItemRadio.h
                ../src/QuicklistMenuItemSeparator.cpp
                ../src/QuicklistMenuItemSeparator.h
                ../src/QuicklistView.cpp
                ../src/QuicklistView.h
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                ../src/Variant.cpp
                ../src/Variant.h
                )

add_executable (test-quicklist-visuals
                ui/TestQuicklistVisuals.cpp
                ui/EventFaker.cpp
                ui/EventFaker.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/QuicklistMenuItem.cpp
                ../src/QuicklistMenuItem.h
                ../src/QuicklistMenuItemCheckmark.cpp
                ../src/QuicklistMenuItemCheckmark.h
                ../src/QuicklistMenuItemLabel.cpp
                ../src/QuicklistMenuItemLabel.h
                ../src/QuicklistMenuItemRadio.cpp
                ../src/QuicklistMenuItemRadio.h
                ../src/QuicklistMenuItemSeparator.cpp
                ../src/QuicklistMenuItemSeparator.h
                ../src/QuicklistView.cpp
                ../src/QuicklistView.h
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                ../src/Variant.cpp
                ../src/Variant.h
                )

add_executable (test-places-backend
                TestPlacesBackend.cpp
                ../src/PlaceEntryRemote.cpp
                ../src/PlaceEntryRemote.h
                ../src/PlaceEntry.h
                ../src/PlaceFactoryFile.cpp
                ../src/PlaceFactoryFile.h
                ../src/PlaceFactory.h
                ../src/PlaceFactory.cpp
                ../src/PlaceRemote.cpp
                ../src/PlaceRemote.h
                ../src/Place.h
                ../src/Variant.cpp
                ../src/Variant.h
                )

add_executable (test-filters
                TestFilters.cpp
                ../src/FilterBasicButton.cpp
                ../src/FilterRatingsWidget.cpp
                ../src/FilterWidget.cpp
                )

add_executable (test-filter-bar
                TestFilterBar.cpp
                ../src/FilterFactory.cpp
                ../src/FilterBasicButton.cpp
                ../src/FilterRatingsWidget.cpp
                ../src/FilterWidget.cpp
                ../src/FilterBar.cpp
                )
#
# GTest tests
#
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
add_executable(test-gtest
               test_timer.cpp
               test_indicator_entry.cpp
               ../src/IndicatorEntry.h
               ../src/IndicatorEntry.cpp
               ../src/Timer.h
               ../src/Timer.cpp
               )
target_link_libraries(test-gtest ${GTEST_BOTH_LIBRARIES})
add_test(UnityGTests test-gtest)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (TEST_COMMAND gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit && ./test-gtest)
add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-gtest)
add_custom_target (gcheck COMMAND  ./test-gtest DEPENDS test-gtest)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})

