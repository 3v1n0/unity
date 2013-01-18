project (test-input-remover)

pkg_check_modules (COMPIZ_TEST_INPUT_REMOVER REQUIRED x11 xext)

if (COMPIZ_TEST_INPUT_REMOVER_FOUND)
	include_directories (${COMPIZ_TEST_INPUT_REMOVER_INCLUDE_DIRS}
			     ../../plugins/unityshell/src)

	link_directories (${COMPIZ_TEST_INPUT_REMOVER_LINK_DIRS})

	add_executable (test-input-remover
			test-input-remover.cpp
			../../plugins/unityshell/src/inputremover.cpp)
        add_dependencies (test-input-remover unity-core-${UNITY_API_VERSION})
	target_link_libraries (test-input-remover
			       ${COMPIZ_TEST_INPUT_REMOVER_LIBRARIES})

endif (COMPIZ_TEST_INPUT_REMOVER_FOUND)
