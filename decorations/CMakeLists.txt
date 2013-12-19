find_package (PkgConfig)
pkg_check_modules (COMPIZ_COMPOSITE REQUIRED "compiz-composite")

set (CFLAGS
     ${COMPIZ_COMPOSITE_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS}
     ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
     ${PIC_FLAGS}
     )

string (REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")
include_directories (.. ../UnityCore ../unity-shared ${CMAKE_BINARY_DIR})

# This makes linker to include library dir in RUNPATH
find_library (COMPIZ_COMPOSITE_LIB composite ${COMPIZ_COMPOSITE_LIBDIR})
set (LIBS ${CACHED_UNITY_DEPS_LDFLAGS} ${COMPIZ_COMPOSITE_LIB} ${COMPIZ_COMPOSITE_LDFLAGS} unity-shared-compiz)

#
# Headers & Sources
#
set (DECORATION_SOURCES
     DecorationsManager.cpp
     DecoratedWindow.cpp
     DecorationsWidgets.cpp
     DecorationsWindowButton.cpp
     DecorationsEdge.cpp
     DecorationsGrabEdge.cpp
     DecorationsEdgeBorders.cpp
     DecorationsInputMixer.cpp
     DecorationsDataPool.cpp
     )

add_library (decorations-lib STATIC ${DECORATION_SOURCES})
target_link_libraries (decorations-lib ${LIBS})
add_dependencies (decorations-lib unity-core-${UNITY_API_VERSION} unity-shared-compiz)
add_pch(pch/decorations_pch.hh decorations-lib)
