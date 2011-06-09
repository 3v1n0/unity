find_package (PkgConfig)
pkg_check_modules (SHARED_DEPS REQUIRED glib-2.0 gio-2.0 sigc++-2.0)

#
# Headers & Sources
#
set (SHARED_HEADERS
     Unity.h
    )

set (SHARED_SOURCES
     Unity.cpp
    )

#
# CFLAGS and LIBS
#
set (CFLAGS
     ${SHARED_DEPS_CFLAGS}
     ${SHARED_DEPS_CFLAGS_OTHER}
     ${MAINTAINER_CFLAGS}
     "-DGETTEXT_PACKAGE=\"unity\""
     "-Werror -Wall"
     )
add_definitions (${CFLAGS})

set (LIBS ${SHARED_DEPS_LIBRARIES})
link_libraries (${LIBS})

set (LIB_PATHS ${SHARED_DEPS_LIBRARY_DIRS})
link_directories(${LIB_PATHS})

#
# Library
#
set (SHARED_LIB_NAME "unity-shared-${UNITY_API_VERSION}")
add_library (${SHARED_LIB_NAME} SHARED ${SHARED_SOURCES})
set_target_properties(${SHARED_LIB_NAME} PROPERTIES VERSION ${UNITY_MAJOR}.0.0 SOVERSION 1)

install (TARGETS ${SHARED_LIB_NAME}
         RUNTIME DESTINATION bin
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
        )

#
# Headers
#
install (FILES ${SHARED_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${SHARED_LIB_NAME}/unity-shared)

#
# PkgConfig file
#
set (EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set (LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set (INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file (unity-shared.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${SHARED_LIB_NAME}.pc)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/${SHARED_LIB_NAME}.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
