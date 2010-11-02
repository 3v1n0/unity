prefix=@PREFIX@
exec_prefix=@DOLLAR@{prefix}
libdir=@DOLLAR@{prefix}/lib
includedir=@DOLLAR@{prefix}/include

Name: unity
Description: Library for developing services integrating with the Unity Shell
Version: @UNITY_VERSION@
Libs: -L@DOLLAR@{libdir} -lunity
Cflags: -I@DOLLAR@{includedir}/unity/unity
Requires: glib-2.0 gthread-2.0 gobject-2.0 gio-2.0 gio-unix-2.0 dbus-glib-1 gtk+-2.0 clutter-1.0 clutk-0.3 dee-1.0 gee-1.0

