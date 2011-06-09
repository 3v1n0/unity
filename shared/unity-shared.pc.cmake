prefix=@PREFIXDIR@
exec_prefix=@EXEC_PREFIX@
libdir=@LIBDIR@
includedir=@INCLUDEDIR@

Name: Unity Shared Library
Description: Core objects and utilities for Unity implementations
Version: @VERSION@
Libs: -L${libdir} -lunity-shared-@UNITY_API_VERSION@
Cflags: -I${includedir}/unity-shared-@UNITY_API_VERSION@
Requires: glib-2.0 gio-2.0 sigc++-2.0
