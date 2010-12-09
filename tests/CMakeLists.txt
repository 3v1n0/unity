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
     "-DGETTEXT_PACKAGE=\"unity\""
     "-DINDICATORDIR=\"${CMAKE_BINARY_DIR}/tests\""
     "-DINDICATORICONDIR=\"${CMAKE_BINARY_DIR}/tests\""
     )
add_definitions (${CFLAGS})

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES})
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${LIB_PATHS})

include_directories (. .. ../services ../src ${CMAKE_BINARY_DIR})

# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../../src or ../../services in alphabetically after that
add_executable (test-unit
                unit/TestFavoriteStoreGSettings.cpp
                unit/TestPanelService.cpp
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
                )

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-unit-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-unit-results.html)
set (TEST_UNIT_COMMAND gtester --verbose -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit)
add_custom_target (check COMMAND  ${TEST_UNIT_COMMAND} DEPENDS test-unit)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
