set(UNITY_CORE_DEPS
    glib-2.0
    gio-2.0
    dee-1.0
    sigc++-2.0
    nux-core-4.0
    unity-protocol-private
)
pkg_check_modules (CORE_DEPS REQUIRED ${UNITY_CORE_DEPS})
pkg_check_modules (PRIVATE_CORE_DEPS REQUIRED unity-protocol-private)

#
# Headers & Sources
#
set (CORE_HEADERS
     ActionHandle.h
     ApplicationPreview.h
     AppmenuIndicator.h
     Categories.h
     Category.h
     CheckOptionFilter.h
     ConnectionManager.h
     DBusIndicators.h
     DesktopUtilities.h
     Filter.h
     Filters.h
     GenericPreview.h
     GLibDBusProxy.h
     GLibDBusServer.h
     GLibSignal.h
     GLibSignal-inl.h
     GLibSource.h
     GLibWrapper.h
     GLibWrapper-inl.h
     GnomeSessionManager.h
     GSettingsScopes.h
     Hud.h
     IndicatorEntry.h
     Indicator.h
     Indicators.h
     MiscUtils.h
     MoviePreview.h
     MultiRangeFilter.h
     MusicPreview.h
     Model.h
     Model-inl.h
     ModelIterator.h
     ModelIterator-inl.h
     ModelRowAdaptor.h
     ModelRowAdaptor-inl.h
     PaymentPreview.h
     Preview.h
     PreviewPlayer.h
     RadioOptionFilter.h
     RatingsFilter.h
     Result.h
     Results.h
     Scope.h
     ScopeData.h
     Scopes.h
     ScopeProxy.h
     ScopeProxyInterface.h
     SessionManager.h
     SocialPreview.h
     Track.h
     Tracks.h
     UWeakPtr.h
     Variant.h
    )

set (CORE_SOURCES
     ApplicationPreview.cpp
     AppmenuIndicator.cpp
     Categories.cpp
     Category.cpp
     ConnectionManager.cpp
     CheckOptionFilter.cpp
     DBusIndicators.cpp
     DesktopUtilities.cpp
     Filter.cpp
     Filters.cpp
     GenericPreview.cpp
     GLibDBusProxy.cpp
     GLibDBusServer.cpp
     GLibSignal.cpp
     GLibSource.cpp
     GLibWrapper.cpp
     GnomeSessionManager.cpp
     GSettingsScopes.cpp
     Hud.cpp
     Indicator.cpp
     IndicatorEntry.cpp
     Indicators.cpp
     MoviePreview.cpp
     MultiRangeFilter.cpp
     MusicPreview.cpp
     ModelRowAdaptor.cpp
     PaymentPreview.cpp
     Preview.cpp
     PreviewPlayer.cpp
     RatingsFilter.cpp
     RadioOptionFilter.cpp
     Result.cpp
     Results.cpp
     Scope.cpp
     ScopeData.cpp
     Scopes.cpp
     ScopeProxy.cpp
     SocialPreview.cpp
     Track.cpp
     Tracks.cpp
     Variant.cpp
    )

#
# CFLAGS and LIBS
#
include_directories(${CORE_DEPS_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR})

set (LIBS ${CORE_DEPS_LDFLAGS} ${PRIVATE_CORE_DEPS_LDFLAGS})

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
set (CORE_LIB_LT_CURRENT 9)
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
add_pch(pch/unitycore_pch.hh ${CORE_LIB_NAME})

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
