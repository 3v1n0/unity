find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unitymtgrabhandles PKGDEPS nux-1.0>=1.2.2 PLUGINDEPS composite opengl CFLAGSADD -std=c++0x)
