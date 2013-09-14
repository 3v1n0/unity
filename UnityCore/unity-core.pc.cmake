prefix=@PREFIXDIR@
exec_prefix=@EXEC_PREFIX@
libdir=@LIBDIR@
includedir=@INCLUDEDIR@

Name: Unity Core
Library
Description: Core objects and utilities for Unity implementations
Version: @VERSION@
Libs: -L${libdir} -lunity-core-@UNITY_API_VERSION@
Cflags: -I${includedir}/Unity-@UNITY_API_VERSION@
Requires: glib-2.0 gio-2.0 sigc++-2.0 nux-core-4.0 dee-1.0
