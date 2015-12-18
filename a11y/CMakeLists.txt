pkg_check_modules(A11Y_DEPS REQUIRED atk atk-bridge-2.0)

set(CFLAGS
    ${CACHED_UNITY_DEPS_CFLAGS}
    ${CACHED_UNITY_DEPS_CFLAGS_OTHER}
    ${PIC_FLAGS}
    ${A11Y_DEPS_CFLAGS})

include_directories(.. ../unity-shared ../dash ../panel ../launcher ../shutdown ../plugins/unityshell/src ${CMAKE_BINARY_DIR})

string(REPLACE ";" " " CFLAGS "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CFLAGS}")
set(LIBS ${CACHED_UNITY_DEPS_LDFLAGS} ${A11Y_DEPS_LDFLAGS})

set(A11Y_SOURCES
    nux-area-accessible.cpp
    nux-base-window-accessible.cpp
    nux-layout-accessible.cpp
    nux-object-accessible.cpp
    nux-text-entry-accessible.cpp
    nux-view-accessible.cpp
    unity-dash-view-accessible.cpp
    unity-expander-view-accessible.cpp
    unity-filter-basic-button-accessible.cpp
    unity-launcher-accessible.cpp
    unity-launcher-icon-accessible.cpp
    unity-panel-view-accessible.cpp
    unity-places-group-accessible.cpp
    unity-quicklist-accessible.cpp
    unity-quicklist-menu-accessible.cpp
    unity-quicklist-menu-item-accessible.cpp
    unity-result-accessible.cpp
    unity-root-accessible.cpp
    unity-rvgrid-accessible.cpp
    unity-scope-bar-icon-accessible.cpp
    unity-sctext-accessible.cpp
    unity-search-bar-accessible.cpp
    unity-session-button-accessible.cpp
    unity-switcher-accessible.cpp
    unity-text-input-accessible.cpp
    unity-util-accessible.cpp
    unitya11y.cpp
    unitya11ytests.cpp
)

add_library(a11y-lib STATIC ${A11Y_SOURCES})
target_link_libraries(a11y-lib ${LIBS})
add_pch(pch/a11y_pch.hh a11y-lib)
