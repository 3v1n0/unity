find_package (PkgConfig)
pkg_check_modules (CORE_DEPS REQUIRED glib-2.0 gio-2.0 sigc++-2.0 nux-core-1.0 gdk-pixbuf-2.0)

#
# Headers & Sources
#
set (CORE_HEADERS
     DBusIndicators.h
     GLibWrapper.h
     GLibWrapper-inl.h
     IndicatorEntry.h
     Indicator.h
     Indicators.h
     UnityCore.h
     Variant.h
    )

set (CORE_SOURCES
     DBusIndicators.cpp
     GLibWrapper.cpp
     Indicator.cpp
     IndicatorEntry.cpp
     Indicators.cpp
     Variant.cpp
    )

#
# CFLAGS and LIBS
#
set (CFLAGS
     ${CORE_DEPS_CFLAGS}
     ${CORE_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-I${CMAKE_BINARY_DIR}"
     "-DGETTEXT_PACKAGE=\"unity\""
     "-Werror -Wall"
     )
add_definitions (${CFLAGS})

set (LIBS ${CORE_DEPS_LIBRARIES})
link_libraries (${LIBS})

set (LIB_PATHS ${CORE_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

#
# Library
#
set (CORE_LIB_NAME "unity-core-${UNITY_API_VERSION}")
add_library (${CORE_LIB_NAME} SHARED ${CORE_SOURCES})
set_target_properties(${CORE_LIB_NAME} PROPERTIES VERSION ${UNITY_MAJOR}.0.0 SOVERSION 1)

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

configure_file (unity-core.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${CORE_LIB_NAME}.pc)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${CORE_LIB_NAME}.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
