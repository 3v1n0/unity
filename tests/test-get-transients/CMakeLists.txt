project (test-input-remover)

pkg_check_modules (COMPIZ_TEST_GET_TRANSIENTS REQUIRED x11 xext)

if (COMPIZ_TEST_GET_TRANSIENTS_FOUND)
	include_directories (${COMPIZ_TEST_GET_TRANSIENTS_INCLUDE_DIRS}
			     ..
			     ../../plugins/unityshell/src)

	link_directories (${COMPIZ_TEST_GET_TRANSIENTS_LINK_DIRS})

	add_executable (test-get-transients
			test-get-transients.cpp
			../../plugins/unityshell/src/transientfor.cpp
			../../plugins/unityshell/src/inputremover.cpp
			../x11-window.cpp
			../x11-window-read-transients.cpp)
        add_dependencies (test-get-transients unity-core-${UNITY_API_VERSION})
	target_link_libraries (test-get-transients
			       ${COMPIZ_TEST_GET_TRANSIENTS_LIBRARIES})

endif (COMPIZ_TEST_GET_TRANSIENTS_FOUND)
