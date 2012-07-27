#ifndef GESTURAL_WINDOW_SWITCHER_MOCK_H
#define GESTURAL_WINDOW_SWITCHER_MOCK_H

#include <Nux/Gesture.h>

class GesturalWindowSwitcherMock : public nux::GestureTarget
{
  public:
    GesturalWindowSwitcherMock() {}

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event)
    {
      return nux::GestureDeliveryRequest::NONE;
    }
};
typedef std::shared_ptr<GesturalWindowSwitcherMock> ShPtGesturalWindowSwitcherMock;

#endif // GESTURAL_WINDOW_SWITCHER_MOCK_H
