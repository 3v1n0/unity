find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (unitymtgrabhandles PLUGINDEPS composite opengl CFLAGSADD -std=c++0x)
