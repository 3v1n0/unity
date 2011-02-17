#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/update-manager.desktop)
set (TESTVALADIR "${CMAKE_SOURCE_DIR}/tests/vala")
#
# Unit tests
#
find_package (PkgConfig)
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};indicator")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS} indicator)

set (CFLAGS
     ${TEST_UNIT_DEPS_CFLAGS}
     ${TEST_UNIT_DEPS_CFLAGS_OTHER}
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
                ../src/FavoriteStore.cpp
                ../src/FavoriteStore.h
                ../src/FavoriteStoreGSettings.cpp
                ../src/FavoriteStoreGSettings.h
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
                ../src/
                )

add_executable (test-panel
                TestPanel.cpp
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
                ../src/IndicatorObjectFactory.h
                ../src/IndicatorObjectProxy.h
                ../src/IndicatorObjectEntryProxy.h
                ../src/IndicatorObjectFactoryRemote.cpp
                ../src/IndicatorObjectFactoryRemote.h
                ../src/IndicatorObjectProxyRemote.cpp
                ../src/IndicatorObjectProxyRemote.h
                ../src/IndicatorObjectEntryProxyRemote.cpp
                ../src/IndicatorObjectEntryProxyRemote.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/PanelHomeButton.cpp
                ../src/PanelHomeButton.h
                ../src/PanelMenuView.cpp
                ../src/PanelMenuView.h
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/WindowButtons.cpp
                ../src/WindowButtons.h
                ../src/WindowManager.cpp
                ../src/WindowManager.h
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                )

add_executable (test-places
                TestPlaces.cpp
                ../src/IconLoader.cpp
                ../src/IconLoader.h
                ../src/IconTexture.cpp
                ../src/IconTexture.h
                ../src/PlacesController.cpp
                ../src/PlacesController.h
                ../src/PlacesHomeView.cpp
                ../src/PlacesHomeView.h
                ../src/PlacesSearchBar.cpp
                ../src/PlacesSearchBar.h
                ../src/PlacesSimpleTile.cpp
                ../src/PlacesSimpleTile.h
                ../src/PlacesTile.cpp
                ../src/PlacesTile.h
                ../src/PlacesResultsController.cpp
                ../src/PlacesResultsController.h
                ../src/PlacesResultsView.h
                ../src/PlacesResultsView.cpp
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                ../src/PlacesView.cpp
                ../src/PlacesView.h
                ../src/TextureCache.h
                ../src/TextureCache.cpp
                ../src/UBusMessages.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
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
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/ubus-server.cpp
                ../src/ubus-server.h
                )

add_executable (test-places-tiles
                TestPlacesTiles.cpp
                ../src/IconLoader.cpp
                ../src/IconLoader.h
                ../src/PlacesTile.cpp
                ../src/PlacesTile.h
                ../src/PlacesSimpleTile.cpp
                ../src/PlacesSimpleTile.h
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/IconTexture.cpp
                ../src/IconTexture.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/TextureCache.h
                ../src/TextureCache.cpp
                )


add_executable (test-places-group
                TestPlacesGroup.cpp
                ../src/IconLoader.cpp
                ../src/IconLoader.h
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                ../src/IconTexture.cpp
                ../src/IconTexture.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/PlacesTile.cpp
                ../src/PlacesTile.h
                ../src/PlacesSimpleTile.cpp
                ../src/PlacesSimpleTile.h
                ../src/TextureCache.h
                ../src/TextureCache.cpp
                )

add_executable (test-places-results
                TestPlacesResults.cpp
                ../src/IconLoader.cpp
                ../src/IconLoader.h
                ../src/PlacesResultsController.cpp
                ../src/PlacesResultsController.h
                ../src/PlacesResultsView.h
                ../src/PlacesResultsView.cpp
                ../src/PlacesGroup.cpp
                ../src/PlacesGroup.h
                ../src/PlacesTile.cpp
                ../src/PlacesTile.h
                ../src/PlacesSimpleTile.cpp
                ../src/PlacesSimpleTile.h
                ../src/IconTexture.cpp
                ../src/IconTexture.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../src/StaticCairoText.cpp
                ../src/StaticCairoText.h
                ../src/TextureCache.h
                ../src/TextureCache.cpp
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
                )
#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (TEST_COMMAND gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit)
add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})

