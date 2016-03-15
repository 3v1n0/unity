find_package (Compiz REQUIRED)

include (CompizPlugin)

# Guard against Compiz altering global state.
# https://bugs.launchpad.net/compiz/+bug/1096807
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(revert_compiz TRUE)
endif()

set (libdir ${CMAKE_INSTALL_LIBDIR})
set (includedir ${CMAKE_INSTALL_INCLUDEDIR})
set (libdir ${CMAKE_INSTALL_LIBDIR})
set (datadir ${CMAKE_INSTALL_FULL_DATADIR})

compiz_plugin (networkarearegion)

if(revert_compiz)
  set (CMAKE_BUILD_TYPE "" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif()

