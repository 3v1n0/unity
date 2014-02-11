project (unity)
cmake_minimum_required(VERSION 2.8.9)

include (cmake/Documentation.cmake)
include (cmake/pch.cmake)

#
# Base bits
#
set (PROJECT_NAME "unity")
set (UNITY_MAJOR 7)
set (UNITY_MINOR 1)
set (UNITY_MICRO 2)
set (UNITY_VERSION "${UNITY_MAJOR}.${UNITY_MINOR}.${UNITY_MICRO}")
set (UNITY_API_VERSION "6.0")
set (UNITY_COMPONENTS_VERSION "6")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGNOME_DESKTOP_USE_UNSTABLE_API -std=c++0x -fno-permissive")
set (CMAKE_CXX_FLAGS_DEBUG "-g3")
set (CMAKE_CXX_FLAGS_RELEASE "")

option(
  ENABLE_X_SUPPORT
  "Enable X.org support in unity"
  ON
)

# This is due to bug lp:668799 - qemu-arm segfaults executing msgmerge
option(
  I18N_SUPPORT
  "Enable I18N, do the .po file thing."
  ON
)

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
  set (PIC_FLAGS "-fPIC")
endif()

if (ENABLE_X_SUPPORT)
  add_definitions(-DUSE_X11)
  message("Unity is configured with support for X.org")
else ()
  message("Unity is configured with without X.org")
  add_definitions(-DNO_X11)
endif ()

if (BUILD_GLES)
  add_definitions(-DNUX_OPENGLES_20)
  add_definitions(-DUSE_GLES)
  set (UNITY_STANDALONE_LADD "unity-core-${UNITY_API_VERSION};m;pthread;dl")
else ()
  set (UNITY_STANDALONE_LADD "unity-core-${UNITY_API_VERSION};m;pthread;dl;GL;GLU")
endif ()

if (CMAKE_BUILD_TYPE MATCHES coverage)
  set (COVERAGE_XML_FILE "${CMAKE_BINARY_DIR}/coverage.xml")
# FIXME: Read below
#  set (COVERAGE_INFO_FILE "${CMAKE_BINARY_DIR}/coverage-html.info")
#  set (COVERAGE_HTML_DIR "${CMAKE_BINARY_DIR}/coverage-html")

  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage" )
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage" )
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} --coverage" )
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage" )

  find_program(GCOVR_EXECUTABLE gcovr HINTS ${GCOVR_ROOT} "${GCOVR_ROOT}/bin")
  if (NOT GCOVR_EXECUTABLE)
      message(FATAL_ERROR "Cannot enable coverage targets because gcovr was not found.")
  else ()
      message (STATUS "Enabling XML coverage report")
      add_custom_target (coverage-xml 
          COMMAND  "${GCOVR_EXECUTABLE}" --exclude="tests.*" --exclude="obj-.*" -x -r "${CMAKE_SOURCE_DIR}" -o "${COVERAGE_XML_FILE}")
  endif()

# FIXME: This got commented out, as it forces a strict dependency on lcov
#  find_program(LCOV_EXECUTABLE lcov HINTS ${LCOV_ROOT} "${LCOV_ROOT}/bin")
#  find_program(GENHTML_EXECUTABLE genhtml HINTS ${GENHTML_ROOT} "${GENHTML_ROOT}/bin")
#  if (NOT LCOV_EXECUTABLE)
#      message(FATAL_ERROR "Cannot enable coverage targets because gcovr was not found.")
#  elseif (NOT GENHTML_EXECUTABLE)
#      message(FATAL_ERROR "Cannot enable coverage targets because genhtml was not found.")
#  else ()
#      message (STATUS "Enabling HTML coverage report")
#      add_custom_target (coverage-html 
#          COMMAND "${LCOV_EXECUTABLE}" --directory "${CMAKE_BINARY_DIR}" --capture --output-file "${COVERAGE_INFO_FILE}"
#          COMMAND "${GENHTML_EXECUTABLE}" --output-directory "${COVERAGE_HTML_DIR}" "${COVERAGE_INFO_FILE}")
#  endif()

endif (CMAKE_BUILD_TYPE MATCHES coverage)


#
# Niceties
#
set (ARCHIVE_NAME unity-${UNITY_VERSION})

add_custom_target (pre-distcheck
  COMMAND echo ""
       && echo "• Releasing Unity ${UNITY_VERSION}"
       && cd ${CMAKE_SOURCE_DIR}
       && echo "• Generating ChangeLog"
       && bzr log --gnu-changelog > ChangeLog
       && echo "• Generating AUTHORS"
       && bzr log --long --levels=0 | grep -e "^\\s*author:" -e "^\\s*committer:" | cut -d ":" -f 2 | sed -r -f AUTHOR-glue | sort -u > AUTHORS
       && echo "• Committing Release"
       && bzr commit -m\"Release ${UNITY_VERSION}\" --unchanged
       && echo "• Tagging Release"
       && bzr tag ${UNITY_VERSION}
       && echo "• Running Distcheck"
  )

add_custom_target (dist
  COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
add_dependencies(dist pre-distcheck)

add_custom_target (distcheck
  COMMAND cd ${CMAKE_BINARY_DIR}
       && rm -rf ${ARCHIVE_NAME}
       && tar xf ${ARCHIVE_NAME}.tar.bz2
       && mkdir ${ARCHIVE_NAME}/build
       && cd ${ARCHIVE_NAME}/build
       && cmake -DCMAKE_INSTALL_PREFIX=../install -DGSETTINGS_LOCALINSTALL=ON .. -DCMAKE_MODULE_PATH=/usr/share/cmake
       && make -j4
       && make -j4 install
       && make check-headless
  )
add_dependencies(distcheck dist)

add_custom_target (post-distcheck
  COMMAND echo "• Signing Tarball"
       && gpg --armor --sign --detach-sig ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
       && echo "• Pushing Branch"
       && bzr push
       && echo "• Publishing Tarball"
       && lp-project-upload unity ${UNITY_VERSION} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
       && echo ""
       && echo "• Unity ${UNITY_VERSION} Released."
  )
add_dependencies(post-distcheck distcheck)

add_custom_target (release)
add_dependencies (release post-distcheck)

#
# config.h
#
set (PREFIXDIR "${CMAKE_INSTALL_PREFIX}")
set (DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set (PKGDATADIR "${DATADIR}/unity/icons")
set (SOURCEDATADIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
set (GETTEXT_PACKAGE "unity")
set (LOCALE_DIR "${DATADIR}/locale")
set (VERSION "${UNITY_VERSION}")
set (BUILDDIR "${CMAKE_BINARY_DIR}")
set (TESTDATADIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/data")

find_package (PkgConfig)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gtk+-3.0 --variable prefix OUTPUT_VARIABLE GTK_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} unity --variable lensesdir OUTPUT_VARIABLE LENSES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} indicator3-0.4 --variable indicatordir OUTPUT_VARIABLE INDICATORDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} indicator3-0.4 --variable iconsdir OUTPUT_VARIABLE INDICATORICONDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} indicator3-0.4 --variable prefix OUTPUT_VARIABLE INDICATORPREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
set (INDICATOR_SERVICE_DIR "${INDICATORPREFIX}/share/unity/indicators")

configure_file (${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

#
# i18n
#

find_package (Gettext REQUIRED)

set (COMPIZ_I18N_DIR ${CMAKE_SOURCE_DIR}/po)

add_custom_command (OUTPUT ${CMAKE_SOURCE_DIR}/po/unity.pot
		COMMAND xgettext -c --from-code=UTF-8 --files-from ${CMAKE_SOURCE_DIR}/po/POTFILES.in --keyword=_ -o ${CMAKE_SOURCE_DIR}/po/unity.pot --copyright-holder="Canonical Ltd" --msgid-bugs-address="ayatana-dev@lists.launchpad.net" --no-wrap --no-location
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)

if (I18N_SUPPORT)
if (GETTEXT_FOUND)
        set (HAVE_GETTEXT true)
        file (GLOB _translations ${CMAKE_SOURCE_DIR}/po/*.po)
        GETTEXT_CREATE_TRANSLATIONS (${CMAKE_SOURCE_DIR}/po/${PROJECT_NAME}.pot
                ALL ${_translations})
endif (GETTEXT_FOUND)
endif()

#
# Enable or disable boot logging
# 
option (BOOT_LOGGER "Enable startup performance logging" OFF)
if (BOOT_LOGGER)
    SET (BOOT_LOGGER_FLAG "-DENABLE_LOGGER")
endif (BOOT_LOGGER)

SET (MAINTAINER_CXXFLAGS "-Werror -Wall -Wcast-align -Wempty-body -Wformat-security -Winit-self -Warray-bounds")

option (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS "Disable errors when local typedefs are unused" ON)
if (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)
  SET (MAINTAINER_CXXFLAGS "${MAINTAINER_CXXFLAGS} -Wno-error=unused-local-typedefs")
endif (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)

option (DISABLE_MAINTAINER_CXXFLAGS "Disable maintainer CXXFlags" OFF)
if (DISABLE_MAINTAINER_CXXFLAGS)
  SET (MAINTAINER_CXXFLAGS "")
endif (DISABLE_MAINTAINER_CXXFLAGS)

# Make sure these flags are used for every build.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MAINTAINER_CXXFLAGS}")

#
# Compiz Plugins
#
set(UNITY_PROTOCOL_PRIVATE_DEPS unity-protocol-private>=7.1.0)
set(UNITY_PLUGIN_SHARED_DEPS
    ${UNITY_PROTOCOL_PRIVATE_DEPS}
    atk
    atk-bridge-2.0
    dbusmenu-glib-0.4
    dee-1.0
    gio-2.0>=2.30.0
    gio-unix-2.0
    gmodule-2.0
    gthread-2.0
    gtk+-3.0>=3.1
    indicator3-0.4>=0.4.90
    json-glib-1.0
    libbamf3>=0.5.0
    gnome-desktop-3.0
    libnotify
    libstartup-notification-1.0
    nux-4.0>=4.0.5
    sigc++-2.0
    unity-misc>=0.4.0
    xpathselect=1.4
    zeitgeist-2.0
)

set(UNITY_PLUGIN_DEPS ${UNITY_PLUGIN_SHARED_DEPS})

if(ENABLE_X_SUPPORT)
  set(UNITY_PLUGIN_DEPS
      ${UNITY_PLUGIN_DEPS}
      compiz>=0.9.9
      libgeis
      x11
      xfixes
      xi>=1.6.99.1
      xrender>=0.9
  )
endif ()

pkg_check_modules (CACHED_UNITY_DEPS REQUIRED ${UNITY_PLUGIN_DEPS})
pkg_check_modules (CACHED_UNITY_PRIVATE_DEPS REQUIRED ${UNITY_PROTOCOL_PRIVATE_DEPS})
find_library (UNITY_PROTOCOL_PRIVATE_LIB unity-protocol-private ${CACHED_UNITY_PRIVATE_DEPS_LIBDIR} ${CACHED_UNITY_PRIVATE_DEPS_LIBRARY_DIRS})
set(UNITY_STANDALONE_LADD ${UNITY_STANDALONE_LADD} ${UNITY_PROTOCOL_PRIVATE_LIB})

add_subdirectory(unity-shared)
add_subdirectory(dash)
add_subdirectory(launcher)
if (ENABLE_X_SUPPORT)
  add_subdirectory(hud)
  add_subdirectory(lockscreen)
  add_subdirectory(panel)
  add_subdirectory(decorations)
  add_subdirectory(plugins/unityshell)
  add_subdirectory(plugins/networkarearegion)
  add_subdirectory(plugins/unitydialog)
  add_subdirectory(plugins/unity-mt-grab-handles)
  add_subdirectory(shortcuts)
  add_subdirectory(shutdown)
  add_subdirectory(unity-standalone)
endif ()

# subdirs
add_subdirectory(doc)

# Check for google test and build it locally
set (
  GMOCK_ROOT_DIR
  "/usr/src/gmock"
  CACHE
  PATH
  "Path to Google mock srcs"
)

set (
  GMOCK_INCLUDE_DIR
  "/usr/include/gmock/include"
  CACHE
  PATH
  "Path to Google mock include"
)

set(
  GTEST_ROOT_DIR
  "${GMOCK_ROOT_DIR}/gtest" # Default value, adjustable by user with e.g., ccmake
  CACHE
  PATH
  "Path to Google test srcs"
)

set(
  GTEST_INCLUDE_DIR
  "${GMOCK_ROOT_DIR}/gtest/include"
  CACHE
  PATH
  "Path to Google tests include"
)

if (GTEST_INCLUDE_DIR AND GMOCK_INCLUDE_DIR)
  include_directories (
    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
  )
  add_subdirectory(
    ${GMOCK_ROOT_DIR}
    "${CMAKE_CURRENT_BINARY_DIR}/gmock"
  )
elseif (GTEST_INCLUDE_DIR AND NOT GMOCK_INCLUDE_DIR)
  include_directories (
    ${GTEST_INCLUDE_DIR}
  )
endif(GTEST_INCLUDE_DIR AND GMOCK_INCLUDE_DIR)

add_subdirectory(services)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(UnityCore)
add_subdirectory(guides)

add_subdirectory(gnome)

#
# GSettings Schema
#
SET (UNITY_SCHEMAS "com.canonical.Unity.gschema.xml")
SET (UNITY_TEST_SCHEMAS "external.gschema.xml")

# Have an option to not install the schema into where GLib is
option (GSETTINGS_LOCALINSTALL "Install GSettings Schemas locally instead of to the GLib prefix" OFF)
if (GSETTINGS_LOCALINSTALL)
    SET (GSETTINGS_DIR "${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas/")
else (GSETTINGS_LOCALINSTALL)
    execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} glib-2.0 --variable prefix  OUTPUT_VARIABLE _glib_prefix OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET (GSETTINGS_DIR "${_glib_prefix}/share/glib-2.0/schemas/")
endif (GSETTINGS_LOCALINSTALL)

# Run the validator and error if it fails
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas  OUTPUT_VARIABLE _glib_comple_schemas OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process (COMMAND ${_glib_comple_schemas} --dry-run --schema-file=${CMAKE_CURRENT_SOURCE_DIR}/${UNITY_SCHEMAS} ERROR_VARIABLE _schemas_invalid OUTPUT_STRIP_TRAILING_WHITESPACE)

if (_schemas_invalid)
  message (SEND_ERROR "Schema validation error: ${_schemas_invalid}")
endif (_schemas_invalid)

# Actually install and recomple schemas
message (STATUS "GSettings schemas will be installed into ${GSETTINGS_DIR}")
install (FILES ${UNITY_SCHEMAS} DESTINATION ${GSETTINGS_DIR} OPTIONAL)
install (CODE "message (STATUS \"Compiling GSettings schemas\")")
install (CODE "execute_process (COMMAND ${_glib_comple_schemas} ${GSETTINGS_DIR})")

# Resources
install (FILES resources/dash-widgets.json DESTINATION ${CMAKE_INSTALL_PREFIX}/share/unity/themes)
file (GLOB _datafiles "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")
install (FILES ${_datafiles} DESTINATION ${PKGDATADIR})


# For testing
configure_file (${CMAKE_SOURCE_DIR}/${UNITY_SCHEMAS} ${CMAKE_BINARY_DIR}/settings/${UNITY_SCHEMAS})
configure_file (${CMAKE_SOURCE_DIR}/tests/data/${UNITY_TEST_SCHEMAS} ${CMAKE_BINARY_DIR}/settings/${UNITY_TEST_SCHEMAS})
execute_process (COMMAND ${_glib_comple_schemas} ${CMAKE_BINARY_DIR}/settings)

#
# docs
#

# check if doxygen is even installed
find_package(Doxygen)
if (DOXYGEN_FOUND STREQUAL "NO")
  message("Doxygen not found. Documentation will not be built")
endif (DOXYGEN_FOUND STREQUAL "NO")

if (DOXYGEN_FOUND STREQUAL "YES")
  set(TOP_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

  # prepare doxygen configuration file
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # add doxygen as target
  add_custom_target(doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # cleanup $build/api-doc on "make clean"
  set_property(DIRECTORY APPEND PROPERTY
         ADDITIONAL_MAKE_CLEAN_FILES api-doc)

endif (DOXYGEN_FOUND STREQUAL "YES")
