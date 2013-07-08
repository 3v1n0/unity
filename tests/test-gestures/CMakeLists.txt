if (GTEST_SRC_DIR)
  set(UNITY_SRC ${CMAKE_SOURCE_DIR}/plugins/unityshell/src)

  add_custom_command(OUTPUT CompoundGestureRecognizer.cpp
                            CompoundGestureRecognizer.h
                            GesturalWindowSwitcher.cpp
                            GesturalWindowSwitcher.h
                            UnityGestureBroker.cpp
                            UnityGestureBroker.h
                            UBusMessages.h
                            WindowGestureTarget.h
                            WindowGestureTarget.cpp

                     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/modify_test_gestures_files.sh 
                                ${CMAKE_CURRENT_SOURCE_DIR} ${UNITY_SRC} ${CMAKE_CURRENT_BINARY_DIR}

                     DEPENDS ${UNITY_SRC}/CompoundGestureRecognizer.cpp
                             ${UNITY_SRC}/CompoundGestureRecognizer.h
                             ${UNITY_SRC}/GesturalWindowSwitcher.cpp
                             ${UNITY_SRC}/GesturalWindowSwitcher.h
                             ${UNITY_SRC}/UnityGestureBroker.cpp
                             ${UNITY_SRC}/UnityGestureBroker.h
                             ${CMAKE_SOURCE_DIR}/unity-shared/UBusMessages.h
                             ${UNITY_SRC}/WindowGestureTarget.h
                             ${UNITY_SRC}/WindowGestureTarget.cpp
                             sed_script_broker
                             sed_script_gesture
                             modify_test_gestures_files.sh

                     COMMENT "Copying and modifying sources under test.")

  # Clean-up includes and definitions made in ../CmakeLists.txt
  remove_definitions(${CFLAGS})
  set_directory_properties(PROPERTY INCLUDE_DIRECTORIES "")

  include_directories (
    ${GMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
  )

  include_directories (${CMAKE_SOURCE_DIR})
  include_directories (${CMAKE_SOURCE_DIR}/unity-shared)

  # And make our own
  pkg_check_modules (TEST_GESTURES_DEPS REQUIRED QUIET "${UNITY_PLUGIN_DEPS}")
  set(TEST_GESTURES_CFLAGS
      "-g"
      "-I${CMAKE_CURRENT_SOURCE_DIR}"
      "-I${CMAKE_CURRENT_BINARY_DIR}"
      ${TEST_GESTURES_DEPS_CFLAGS}
     )
  add_definitions(${TEST_GESTURES_CFLAGS})

  pkg_check_modules (COMPIZ REQUIRED QUIET compiz)
  link_directories (${COMPIZ_LIBDIR})

  add_executable(test-gestures
                 CompoundGestureRecognizer.cpp
                 CompoundGestureRecognizer.h
                 GesturalWindowSwitcher.cpp
                 GesturalWindowSwitcher.h
                 test_compound_gesture_recognizer.cpp
                 test_gestural_window_switcher.cpp
                 test_gestures_main.cpp
                 test_gesture_broker.cpp
                 test_window_gesture_target.cpp
                 X11_mock.cpp
                 UnityGestureBroker.cpp
                 WindowGestureTarget.cpp
                 PluginAdapterMock.cpp
                 ubus-server-mock.cpp
                 UnityGestureTargetMock.h
                )
  target_link_libraries(test-gestures gtest ${TEST_GESTURES_DEPS_LIBRARIES} compiz_core unity-core-${UNITY_API_VERSION})
  add_test(UnityGTestGestures test-gestures)
  add_dependencies(test-gestures gtest unity-core-${UNITY_API_VERSION})

  add_custom_target (check-gestures COMMAND ./test-gestures DEPENDS test-gestures)
endif (GTEST_SRC_DIR)
