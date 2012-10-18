find_package (PkgConfig)
pkg_check_modules (CORE_DEPS REQUIRED glib-2.0 gio-2.0 dee-1.0 sigc++-2.0 nux-core-3.0 gdk-pixbuf-2.0 unity-protocol-private)
pkg_check_modules (PRIVATE_CORE_DEPS REQUIRED unity-protocol-private)

execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} unity --variable lensesdir OUTPUT_VARIABLE _lensesdir OUTPUT_STRIP_TRAILING_WHITESPACE)

#
# Headers & Sources
#
set (CORE_HEADERS
     ApplicationPreview.h
     AppmenuIndicator.h
     Categories.h
     Category.h
     CheckOptionFilter.h
     DBusIndicators.h
     DesktopUtilities.h
     FilesystemLenses.h
     Filter.h
     Filters.h
     GenericPreview.h
     GLibDBusProxy.h
     GLibSignal.h
     GLibSignal-inl.h
     GLibSource.h
     GLibWrapper.h
     GLibWrapper-inl.h
     Hud.h
     HomeLens.h
     IndicatorEntry.h
     Indicator.h
     Indicators.h
     Lens.h
     Lenses.h
     MoviePreview.h
     MultiRangeFilter.h
     MusicPreview.h
     Model.h
     Model-inl.h
     ModelRowAdaptor.h
     ModelRowAdaptor-inl.h
     Preview.h
     RadioOptionFilter.h
     RatingsFilter.h
     Result.h
     ResultIterator.h
     Results.h
     SeriesPreview.h
     SocialPreview.h
     Track.h
     Tracks.h
     Variant.h
     U1PaymentPreview.h
    )

set (CORE_SOURCES
     ApplicationPreview.cpp
     AppmenuIndicator.cpp
     Categories.cpp
     Category.cpp
     CheckOptionFilter.cpp
     DBusIndicators.cpp
     DesktopUtilities.cpp
     FilesystemLenses.cpp
     Filter.cpp
     Filters.cpp
     GenericPreview.cpp
     GLibDBusProxy.cpp
     GLibSignal.cpp
     GLibSource.cpp
     GLibWrapper.cpp
     Hud.cpp
     HomeLens.cpp
     Indicator.cpp
     IndicatorEntry.cpp
     Indicators.cpp
     Lens.cpp
     MoviePreview.cpp
     MultiRangeFilter.cpp
     MusicPreview.cpp
     ModelRowAdaptor.cpp
     Preview.cpp
     RatingsFilter.cpp
     RadioOptionFilter.cpp
     Result.cpp
     ResultIterator.cpp
     Results.cpp
     SeriesPreview.cpp
     SocialPreview.cpp
     Track.cpp
     Tracks.cpp
     Variant.cpp
     U1PaymentPreview.cpp
    )

#
# CFLAGS and LIBS
#
set (CFLAGS
     "-I${CMAKE_BINARY_DIR}"
     "-DGETTEXT_PACKAGE=\"unity\""
     "-DLENSES_DIR=\"${_lensesdir}\""
     )
add_definitions (${CFLAGS})
include_directories(${CORE_DEPS_INCLUDE_DIRS})

set (LIBS ${CORE_DEPS_LIBRARIES})

set (LIB_PATHS ${CORE_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

#
# Library
#
# Before making a release, the version string should be updated.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
set (CORE_LIB_LT_CURRENT 5)
set (CORE_LIB_LT_REV 0)
set (CORE_LIB_LT_AGE 0)
set (CORE_LIB_LT_VERSION "${CORE_LIB_LT_CURRENT}:${CORE_LIB_LT_REV}:${CORE_LIB_LT_AGE}")

set (CORE_LIB_NAME "unity-core-${UNITY_API_VERSION}")

add_library (${CORE_LIB_NAME} SHARED ${CORE_SOURCES})
target_link_libraries (${CORE_LIB_NAME} ${LIBS})
set_target_properties(${CORE_LIB_NAME} PROPERTIES
                      VERSION ${CORE_LIB_LT_CURRENT}.${CORE_LIB_LT_REV}.${CORE_LIB_LT_AGE}
                      SOVERSION ${CORE_LIB_LT_CURRENT}
                      INSTALL_RPATH "${PRIVATE_CORE_DEPS_LIBRARY_DIRS}")

install (TARGETS ${CORE_LIB_NAME}
         RUNTIME DESTINATION bin
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
        )

#
# Headers
#
install (FILES ${CORE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/Unity-${UNITY_API_VERSION}/UnityCore)

#
# PkgConfig file
#
set (EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set (LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set (INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file (unity-core.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${CORE_LIB_NAME}.pc @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${CORE_LIB_NAME}.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
