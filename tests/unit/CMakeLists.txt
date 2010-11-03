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

include_directories (. .. ../../src)

# We can't have convenience libs so we need to rebuild with what we need
add_executable (test-unit
                test-unit.cpp)
