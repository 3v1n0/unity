find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (gtkloader PKGDEPS gtk+-3.0)
