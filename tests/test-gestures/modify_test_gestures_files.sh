#!/bin/bash

CMAKE_CURRENT_SOURCE_DIR=${1}
UNITY_SRC=${2}
CMAKE_CURRENT_BINARY_DIR=${3}

cp -a ${UNITY_SRC}/CompoundGestureRecognizer.cpp \
      ${UNITY_SRC}/CompoundGestureRecognizer.h \
      ${CMAKE_CURRENT_BINARY_DIR}

if [ ${UNITY_SRC}/UnityGestureBroker.cpp -nt ${CMAKE_CURRENT_BINARY_DIR}/UnityGestureBroker.cpp ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_broker \
    ${UNITY_SRC}/UnityGestureBroker.cpp > ${CMAKE_CURRENT_BINARY_DIR}/UnityGestureBroker.cpp
fi

if [ ${UNITY_SRC}/UnityGestureBroker.h -nt ${CMAKE_CURRENT_BINARY_DIR}/UnityGestureBroker.h ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_broker \
    ${UNITY_SRC}/UnityGestureBroker.h > ${CMAKE_CURRENT_BINARY_DIR}/UnityGestureBroker.h
fi

if [ ${UNITY_SRC}/WindowGestureTarget.h -nt ${CMAKE_CURRENT_BINARY_DIR}/WindowGestureTarget.h ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_gesture \
    ${UNITY_SRC}/WindowGestureTarget.h > ${CMAKE_CURRENT_BINARY_DIR}/WindowGestureTarget.h
fi

if [ ${UNITY_SRC}/WindowGestureTarget.cpp -nt ${CMAKE_CURRENT_BINARY_DIR}/WindowGestureTarget.cpp ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_gesture \
    ${UNITY_SRC}/WindowGestureTarget.cpp > ${CMAKE_CURRENT_BINARY_DIR}/WindowGestureTarget.cpp
fi

if [ ${UNITY_SRC}/GesturalWindowSwitcher.cpp -nt ${CMAKE_CURRENT_BINARY_DIR}/GesturalWindowSwitcher.cpp ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_switcher \
    ${UNITY_SRC}/GesturalWindowSwitcher.cpp > ${CMAKE_CURRENT_BINARY_DIR}/GesturalWindowSwitcher.cpp
fi

if [ ${UNITY_SRC}/GesturalWindowSwitcher.h -nt ${CMAKE_CURRENT_BINARY_DIR}/GesturalWindowSwitcher.h ]
then
  sed -f ${CMAKE_CURRENT_SOURCE_DIR}/sed_script_switcher \
    ${UNITY_SRC}/GesturalWindowSwitcher.h > ${CMAKE_CURRENT_BINARY_DIR}/GesturalWindowSwitcher.h
fi

exit 0
