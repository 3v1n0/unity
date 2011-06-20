find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unitydialog PLUGINDEPS composite opengl PKGDEPS cairo cairo-xlib-xrender gtk+-2.0)

