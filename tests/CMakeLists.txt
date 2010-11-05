#
# Unit tests
#
find_package (PkgConfig)
pkg_check_modules (TEST_UNIT_DEPS REQUIRED ${UNITY_PLUGIN_DEPS})

set (CFLAGS ${TEST_UNIT_DEPS_CFLAGS} ${TEST_UNIT_DEPS_CFLAGS_OTHER})
add_definitions (${CFLAGS})

set (LIBS ${TEST_UNIT_DEPS_LIBRARIES})
link_libraries (${LIBS})

set (LIB_PATHS ${TEST_UNIT_DEPS_LIBRARY_DIRS})
link_directories (${LIB_PATHS})

include_directories (. .. ../src ${CMAKE_BINARY_DIR})

# We can't have convenience libs so we need to rebuild with what we need
# Please keep actual test files alphabetically at top and then files
# from ../../src in alphabetically after that
add_executable (test-unit
                unit/TestFavoriteStoreGSettings.cpp
                unit/TestMain.cpp
                ../src/FavoriteStore.cpp
                ../src/FavoriteStore.h
                ../src/FavoriteStoreGSettings.cpp
                ../src/FavoriteStoreGSettings.h)

#
# check target
#
set (TEST_RESULT_DIR ${CMAKE_BINARY_DIR}/tests)
set (TEST_RESULT_XML ${TEST_RESULT_DIR}/test-unit-results.xml)
set (TEST_RESULT_HTML ${TEST_RESULT_DIR}/test-unit-results.html)
set (TEST_UNIT_COMMAND gtester -k --g-fatal-warnings -o=${TEST_RESULT_XML} ./test-unit)
add_custom_target (check COMMAND  ${TEST_UNIT_COMMAND} DEPENDS test-unit)
add_custom_target (check-report COMMAND ${TEST_UNIT_COMMAND} && gtester-report ${TEST_RESULT_XML} > ${TEST_RESULT_HTML})
