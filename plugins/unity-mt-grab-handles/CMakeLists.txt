find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unitymtgrabhandles PKGDEPS nux-2.0 >= 2.0.0 PLUGINDEPS composite opengl CFLAGSADD -std=c++0x)
