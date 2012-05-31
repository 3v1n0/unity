if (GTEST_SRC_DIR)
  set(UNITY_SRC ${CMAKE_SOURCE_DIR}/plugins/unityshell/src)

  add_custom_command(OUTPUT GestureEngine.cpp GestureEngine.h UBusMessages.h
                     COMMAND cp ${UNITY_SRC}/GestureEngine.cpp ${UNITY_SRC}/GestureEngine.h ${CMAKE_SOURCE_DIR}/unity-shared//UBusMessages.h ${CMAKE_CURRENT_BINARY_DIR}
                     COMMAND sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script ${UNITY_SRC}/GestureEngine.cpp > ${CMAKE_CURRENT_BINARY_DIR}/GestureEngine.cpp
                     COMMAND sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script ${UNITY_SRC}/GestureEngine.h > ${CMAKE_CURRENT_BINARY_DIR}/GestureEngine.h
                     DEPENDS ${UNITY_SRC}/GestureEngine.cpp ${UNITY_SRC}/GestureEngine.h ${CMAKE_SOURCE_DIR}/unity-shared/UBusMessages.h
                     COMMENT "Copying GestureEngine source.")

  # Clean-up includes and definitions made in ../CmakeLists.txt
  remove_definitions(${CFLAGS})
  set_directory_properties(PROPERTY INCLUDE_DIRECTORIES "")
  # And make our own
  pkg_check_modules (TEST_GESTURE_ENGINE_DEPS REQUIRED QUIET "${UNITY_PLUGIN_DEPS}")
  set(TEST_GESTURE_ENGINE_CFLAGS
      "-g"
      "-I${CMAKE_CURRENT_SOURCE_DIR}"
      "-I${CMAKE_CURRENT_BINARY_DIR}"
      ${TEST_GESTURE_ENGINE_DEPS_CFLAGS}
     )
  add_definitions(${TEST_GESTURE_ENGINE_CFLAGS})

  pkg_check_modules (COMPIZ REQUIRED QUIET compiz)
  link_directories (${COMPIZ_LIBDIR})

  add_executable(test-gesture-engine
                 test_gesture_engine.cpp
                 X11_mock.cpp
                 GestureEngine.cpp
                 PluginAdapterMock.cpp
                 GeisAdapterMock.cpp
                 ubus-server-mock.cpp
                )
  target_link_libraries(test-gesture-engine gtest ${TEST_GESTURE_ENGINE_DEPS_LIBRARIES} -lcompiz_core)
  add_test(UnityGTestGestureEngine test-gesture-engine)
  add_dependencies(test-gesture-engine gtest unity-core-${UNITY_API_VERSION})

  add_custom_target (check-gesture-engine COMMAND ./test-gesture-engine DEPENDS test-gesture-engine)
endif (GTEST_SRC_DIR)
