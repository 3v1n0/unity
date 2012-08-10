#ifndef WINDOW_GESTURE_TARGET_MOCK_H
#define WINDOW_GESTURE_TARGET_MOCK_H

#include <Nux/Gesture.h>

#include <set>

class CompWindowMock;
class WindowGestureTargetMock;

extern std::set<WindowGestureTargetMock*> g_window_target_mocks;

class WindowGestureTargetMock : public nux::GestureTarget
{
  public:
    WindowGestureTargetMock(CompWindowMock *window) : window(window)
    {
      g_window_target_mocks.insert(this);
    }

    virtual ~WindowGestureTargetMock()
    {
      g_window_target_mocks.erase(this);
    }

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event)
    {
      events_received.push_back(event);
      return nux::GestureDeliveryRequest::NONE;
    }

    CompWindowMock *window;
    std::list<nux::GestureEvent> events_received;

    static Cursor fleur_cursor;
  private:
    virtual bool Equals(const nux::GestureTarget& other) const
    {
      const WindowGestureTargetMock *window_target = dynamic_cast<const WindowGestureTargetMock *>(&other);

      if (window_target)
      {
        if (window && window_target->window)
          return window->id() == window_target->window->id();
        else
          return window == window_target->window;
      }
      else
      {
        return false;
      }
    }
};

#endif // WINDOW_GESTURE_TARGET_MOCK_H
