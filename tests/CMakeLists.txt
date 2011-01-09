#
# Data
#
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/data/update-manager.desktop
                ${CMAKE_BINARY_DIR}/tests/data/update-manager.desktop)

#
# Unit tests
#
find_package (PkgConfig)
set (TEST_DEPS "${UNITY_PLUGIN_DEPS};indicator;gee-1.0")
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${TEST_DEPS} indicator)

set (CFLAGS
     ${TEST_UNIT_DEPS_CFLAGS}
     ${TEST_UNIT_DEPS_CFLAGS_OTHER}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-DINDICATORDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-DINDICATORICONDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-I${CMAKE_BINARY_DIR}/libunity"
     )
add_definitions (${CFLAGS})

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES} unity)
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS} ${CMAKE_BINARY_DIR}/libunity)
link_directories (${LIB_PATHS})

include_directories (. .. ${CMAKE_BINARY_DIR}/libunity ../services ${CMAKE_BINARY_DIR})

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
                ../libunity/ubus-server.c
                ../libunity/ubus-server.h
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
                ../src/PanelView.cpp
                ../src/PanelView.h
                ../src/PanelIndicatorObjectView.cpp
                ../src/PanelIndicatorObjectView.h
                ../src/PanelIndicatorObjectEntryView.cpp
                ../src/PanelIndicatorObjectEntryView.h
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
                ../libunity/ubus-server.c
                ../libunity/ubus-server.h
                )

add_executable (test-places
                TestPlaces.cpp
                ../src/PlacesController.cpp
                ../src/PlacesController.h
                ../src/PlacesView.cpp
                ../src/PlacesView.h
                ../src/UBusMessages.h
                ../src/Introspectable.cpp
                ../src/Introspectable.h
                ../libunity/ubus-server.c
                ../libunity/ubus-server.h
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
                )

add_executable (test-places-backend
                TestPlacesBackend.cpp
                ../src/PlaceEntryRemote.cpp
                ../src/PlaceEntryRemote.h
                ../src/PlaceEntry.h
                ../src/PlaceFactoryFile.cpp
                ../src/PlaceFactoryFile.h
                ../src/PlaceFactory.h
                ../src/PlaceRemote.cpp
                ../src/PlaceRemote.h
                ../src/Place.h
                )
#
# Vala tests
#
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/vala)
include(ValaPrecompile)
include(ValaVersion)

find_package (Vala)

vala_precompile (VALA_C vala/test-unit.vala
                        vala/test-place-browser.vala
                        vala/test-appinfo-manager.vala
                        vala/test-io.vala
                        vala/test-place.vala
                        vala/test-utils.vala
                 PACKAGES gtk+-2.0
                          gio-2.0
                          glib-2.0
                          gobject-2.0
                          gee-1.0
                          dbus-glib-1
                          gee-1.0
                 OPTIONS --thread
                 CUSTOM_VAPIS ${CMAKE_SOURCE_DIR}/vapi/dee-1.0.vapi
                              vala/test-const.vapi
                              vala/config.vapi
                              ${CMAKE_BINARY_DIR}/libunity/unity.vapi
)

add_executable(test-vala ${VALA_C} vala/numpty.cpp)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-results.html)
set (TEST_COMMAND gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit ./test-vala)
add_custom_target (check COMMAND  ${TEST_COMMAND} DEPENDS test-unit test-vala)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
