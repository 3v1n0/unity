find_package (Compiz REQUIRED)

include (CompizPlugin)

# Guard against Compiz altering global state.
# https://bugs.launchpad.net/compiz/+bug/1096807
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(revert_compiz TRUE)
endif()

compiz_plugin (unitymtgrabhandles PKGDEPS nux-4.0>=4.0.0 PLUGINDEPS composite opengl CFLAGSADD -std=c++0x)

if(revert_compiz)
  set (CMAKE_BUILD_TYPE "" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRe)" FORCE)
endif()

