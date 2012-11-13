find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unitymtgrabhandles PKGDEPS nux-4.0>=4.0.0 PLUGINDEPS composite opengl CFLAGSADD -std=c++0x)
