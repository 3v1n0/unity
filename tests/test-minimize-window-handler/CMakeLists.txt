project (test-input-remover)

pkg_check_modules (COMPIZ_TEST_MINIMIZED_HANDER REQUIRED x11 xext)

if (COMPIZ_TEST_MINIMIZED_HANDER_FOUND)
	include_directories (${COMPIZ_TEST_MINIMIZED_HANDER_INCLUDE_DIRS}
			     ..
			     ../../plugins/unityshell/src)

	link_directories (${COMPIZ_TEST_MINIMIZED_HANDER_LINK_DIRS})

	add_executable (test-minimize-handler
			test-minimize-handler.cpp
			../../plugins/unityshell/src/minimizedwindowhandler.cpp
			../x11-window.cpp
			../x11-window-read-transients.cpp
			../../plugins/unityshell/src/transientfor.cpp
			../../plugins/unityshell/src/inputremover.cpp)
        add_dependencies (test-minimize-handler unity-core-${UNITY_API_VERSION})
	target_link_libraries (test-minimize-handler
			       ${COMPIZ_TEST_MINIMIZED_HANDER_LIBRARIES})

endif (COMPIZ_TEST_MINIMIZED_HANDER_FOUND)
