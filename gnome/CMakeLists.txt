find_package (Compiz REQUIRED)

include (CompizCommon)

compiz_translate_xml (${CMAKE_CURRENT_SOURCE_DIR}/50-unity-launchers.xml.in
		      ${CMAKE_CURRENT_BINARY_DIR}/50-unity-launchers.xml NOTRANSLATIONS)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/50-unity-launchers.xml
	 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gnome-control-center/keybindings)

add_custom_target (unity-gnome-keybindings ALL DEPENDS
		   ${CMAKE_CURRENT_BINARY_DIR}/50-unity-launchers.xml)
