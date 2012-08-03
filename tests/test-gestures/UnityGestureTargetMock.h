#ifndef UNITY_GESTURE_TARGET_MOCK_H
#define UNITY_GESTURE_TARGET_MOCK_H

#include <Nux/Gesture.h>

class UnityGestureTargetMock : public nux::GestureTarget
{
  public:
    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event)
    {
      return nux::GestureDeliveryRequest::NONE;
    }
};

#endif // UNITY_GESTURE_TARGET_MOCK_H
