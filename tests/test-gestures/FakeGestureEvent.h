#ifndef FAKE_GESTURE_EVENT_H
#define FAKE_GESTURE_EVENT_H

#include <NuxGraphics/GestureEvent.h>
#include <map>

namespace nux {
class FakeGestureEvent
{
  public:
    nux::EventType type;

    int gesture_id;
    int gesture_classes;
    bool is_direct_touch;
    int timestamp;
    nux::Point2D<float> focus;
    nux::Point2D<float> delta;
    float angle;
    float angle_delta;
    float angular_velocity;
    int tap_duration;
    nux::Point2D<float> velocity;
    float radius;
    float radius_delta;
    float radial_velocity;
    std::vector<nux::TouchPoint> touches;
    bool is_construction_finished;

    nux::GestureEvent &ToGestureEvent()
    {
      event_.type = type;

      event_.gesture_id_ = gesture_id;
      event_.gesture_classes_ = gesture_classes;
      event_.is_direct_touch_ = is_direct_touch;
      event_.timestamp_ = timestamp;
      event_.focus_ = focus;
      event_.delta_ = delta;
      event_.angle_ = angle;
      event_.angle_delta_ = angle_delta;
      event_.angular_velocity_ = angular_velocity;
      event_.tap_duration_ = tap_duration;
      event_.velocity_ = velocity;
      event_.radius_ = radius;
      event_.radius_delta_ = radius_delta;
      event_.radial_velocity_ = radial_velocity;
      event_.touches_ = touches;
      event_.is_construction_finished_ = is_construction_finished;

      return event_;
    }

  private:
    nux::GestureEvent event_;
};
} // namespace nux

// maps a gesture id to its acceptance
extern std::map<int, int> g_gesture_event_accept_count;
extern std::map<int, int> g_gesture_event_reject_count;

#endif // FAKE_GESTURE_EVENT_H
