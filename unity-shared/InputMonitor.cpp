// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "InputMonitor.h"
#include "SigcSlotHash.h"

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <X11/extensions/XInput2.h>
#include <UnityCore/GLibSource.h>
#include <unordered_set>
#include <gdk/gdkx.h>
#include <glib.h>

namespace unity
{
namespace input
{
namespace
{
DECLARE_LOGGER(logger, "unity.input.monitor");

Monitor* instance_ = nullptr;

const unsigned XINPUT_MAJOR_VERSION = 2;
const unsigned XINPUT_MINOR_VERSION = 3;

bool operator&(Events l, Events r)
{
  typedef std::underlying_type<Events>::type ut;
  return static_cast<ut>(static_cast<ut>(l) & static_cast<ut>(r));
}

Events& operator|=(Events& l, Events r)
{
  typedef std::underlying_type<Events>::type ut;
  return l = static_cast<Events>(static_cast<ut>(l) | static_cast<ut>(r));
}

template <typename EVENT>
void initialize_event_common(EVENT* ev, XIDeviceEvent* xiev)
{
  ev->serial = xiev->serial;
  ev->send_event = xiev->send_event;
  ev->display = xiev->display;
  ev->window = xiev->event;
  ev->root = xiev->root;
  ev->subwindow = xiev->child;
  ev->time = xiev->time;
  ev->x = std::round(xiev->event_x);
  ev->y = std::round(xiev->event_y);
  ev->x_root = std::round(xiev->root_x);
  ev->y_root = std::round(xiev->root_y);
  ev->state = xiev->mods.effective;
  ev->same_screen = True;
}

template <typename EVENT_TYPE, typename NATIVE_TYPE>
void initialize_event(XEvent* ev, NATIVE_TYPE* xiev);

template <>
void initialize_event<XButtonEvent>(XEvent* ev, XIDeviceEvent* xiev)
{
  XButtonEvent* bev = &ev->xbutton;
  ev->type = (xiev->evtype == XI_ButtonPress) ? ButtonPress : ButtonRelease;
  initialize_event_common(bev, xiev);
  bev->button = xiev->detail;
}

template <>
void initialize_event<XKeyEvent>(XEvent* ev, XIDeviceEvent* xiev)
{
  XKeyEvent* kev = &ev->xkey;
  ev->type = (xiev->evtype == XI_KeyPress) ? KeyPress : KeyRelease;
  initialize_event_common(kev, xiev);
  kev->keycode = xiev->detail;
}

template <>
void initialize_event<XMotionEvent>(XEvent* ev, XIDeviceEvent* xiev)
{
  XMotionEvent* mev = &ev->xmotion;
  ev->type = MotionNotify;
  initialize_event_common(mev, xiev);
  mev->is_hint = NotifyNormal;

  for (int i = 0; i < xiev->buttons.mask_len * 8; ++i)
  {
    if (XIMaskIsSet(xiev->buttons.mask, i))
    {
      mev->is_hint = NotifyHint;
      break;
    }
  }
}

template <>
void initialize_event<XGenericEventCookie>(XEvent* ev, XIBarrierEvent* xiev)
{
  XGenericEventCookie* cev = &ev->xcookie;
  cev->type = GenericEvent;
  cev->serial = xiev->serial;
  cev->send_event = xiev->send_event;
  cev->display = xiev->display;
  cev->evtype = xiev->evtype;
  cev->data = xiev;
}
}

struct Monitor::Impl
{
#if __GNUC__ < 6
  using EventCallbackSet = std::unordered_set<EventCallback>;
#else
  using EventCallbackSet = std::unordered_set<EventCallback, std::hash<sigc::slot_base>>;
#endif

  Impl()
    : xi_opcode_(0)
    , event_filter_set_(false)
    , invoking_callbacks_(false)
  {
    Display *dpy = gdk_x11_get_default_xdisplay();
    int event_base, error_base;

    if (XQueryExtension(dpy, "XInputExtension", &xi_opcode_, &event_base, &error_base))
    {
      int maj = XINPUT_MAJOR_VERSION;
      int min = XINPUT_MINOR_VERSION;

      if (XIQueryVersion(dpy, &maj, &min) == BadRequest)
      {
        LOG_ERROR(logger) << "Need XInput version "<< maj << "." << min << ", "
                          << "impossible, to setup an InputMonitor";
      }
    }
    else
    {
      LOG_ERROR(logger) << "Missing XInput, impossible to setup an InputMonitor";
    }
  }

  ~Impl()
  {
    if (event_filter_set_)
    {
      pointer_callbacks_.clear();
      key_callbacks_.clear();
      barrier_callbacks_.clear();
      UpdateEventMonitor();
    }
  }

  bool RegisterClient(Events type, EventCallback const& cb)
  {
    bool added = false;

    if (type & Events::POINTER)
      added = pointer_callbacks_.insert(cb).second || added;

    if (type & Events::KEYS)
      added = key_callbacks_.insert(cb).second || added;

    if (type & Events::BARRIER)
      added = barrier_callbacks_.insert(cb).second || added;

    if (added)
      UpdateEventMonitor();

    return added;
  }

  bool UnregisterClient(EventCallback const& cb)
  {
    if (invoking_callbacks_)
    {
      // Delay the event removal if we're currently invoking a callback
      // not to break the callbacks loop
      removal_queue_.insert(cb);
      return false;
    }

    bool removed = false;
    removed = pointer_callbacks_.erase(cb) > 0 || removed;
    removed = key_callbacks_.erase(cb) > 0 || removed;
    removed = barrier_callbacks_.erase(cb) > 0 || removed;

    if (removed)
      UpdateEventMonitor();

    return removed;
  }

  Events RegisteredEvents(EventCallback const& cb) const
  {
    Events events = Events::NONE;

    if (pointer_callbacks_.find(cb) != end(pointer_callbacks_))
      events |= Events::POINTER;

    if (key_callbacks_.find(cb) != end(key_callbacks_))
      events |= Events::KEYS;

    if (barrier_callbacks_.find(cb) != end(barrier_callbacks_))
      events |= Events::BARRIER;

    return events;
  }

  void UpdateEventMonitor()
  {
    auto* nux_dpy = nux::GetGraphicsDisplay();
    auto* dpy = nux_dpy ? nux_dpy->GetX11Display() : gdk_x11_get_default_xdisplay();
    Window root = DefaultRootWindow(dpy);

    unsigned char master_dev_bits[XIMaskLen(XI_LASTEVENT)] = { 0 };
    XIEventMask master_dev = { XIAllMasterDevices, sizeof(master_dev_bits), master_dev_bits };

    if (!barrier_callbacks_.empty())
    {
      XISetMask(master_dev.mask, XI_BarrierHit);
      XISetMask(master_dev.mask, XI_BarrierLeave);
    }

    unsigned char all_devs_bits[XIMaskLen(XI_LASTEVENT)] = { 0 };
    XIEventMask all_devs = { XIAllDevices, sizeof(all_devs_bits), all_devs_bits };

    if (!pointer_callbacks_.empty())
    {
      XISetMask(all_devs.mask, XI_Motion);
      XISetMask(all_devs.mask, XI_ButtonPress);
      XISetMask(all_devs.mask, XI_ButtonRelease);
    }

    if (!key_callbacks_.empty())
    {
      XISetMask(all_devs.mask, XI_KeyPress);
      XISetMask(all_devs.mask, XI_KeyRelease);
    }

    XIEventMask selected[] = {master_dev, all_devs};
    XISelectEvents(dpy, root, selected, G_N_ELEMENTS(selected));
    XSync(dpy, False);

    LOG_DEBUG(logger) << "Pointer clients: " << pointer_callbacks_.size() << ", "
                      << "Key clients: " << key_callbacks_.size() << ", "
                      << "Barrier clients: " << barrier_callbacks_.size();

    if (!pointer_callbacks_.empty() || !key_callbacks_.empty() || !barrier_callbacks_.empty())
    {
      if (!event_filter_set_ && nux_dpy)
      {
        nux_dpy->AddEventFilter({[] (XEvent event, void* data) {
          return static_cast<Impl*>(data)->HandleEvent(event);
        }, this});

        event_filter_set_ = true;
        LOG_DEBUG(logger) << "Event filter enabled";
      }
    }
    else if (event_filter_set_)
    {
      if (nux_dpy)
        nux_dpy->RemoveEventFilter(this);

      event_filter_set_ = false;
      LOG_DEBUG(logger) << "Event filter disabled";
    }
  }

  bool HandleEvent(XEvent& event)
  {
    bool handled = false;

    if (event.type != GenericEvent || event.xcookie.extension != xi_opcode_)
      return handled;

    switch (event.xcookie.evtype)
    {
      case XI_ButtonPress:
      case XI_ButtonRelease:
        handled = InvokeCallbacks<XButtonEvent>(pointer_callbacks_, event);
        break;
      case XI_Motion:
        handled = InvokeCallbacks<XMotionEvent>(pointer_callbacks_, event);
        break;
      case XI_KeyPress:
      case XI_KeyRelease:
        handled = InvokeCallbacks<XKeyEvent>(key_callbacks_, event);
        break;
      case XI_BarrierHit:
      case XI_BarrierLeave:
        handled = InvokeCallbacks<XGenericEventCookie, XIBarrierEvent>(barrier_callbacks_, event);
        break;
    }

    return handled;
  }

  template <typename EVENT_TYPE, typename NATIVE_TYPE = XIDeviceEvent>
  bool InvokeCallbacks(EventCallbackSet& callbacks, XEvent& xiev)
  {
    XGenericEventCookie *cookie = &xiev.xcookie;

    if (!XGetEventData(xiev.xany.display, cookie))
      return false;

    XEvent event;
    initialize_event<EVENT_TYPE>(&event, reinterpret_cast<NATIVE_TYPE*>(cookie->data));
    invoking_callbacks_ = true;

    for (auto it = callbacks.begin(); it != callbacks.end();)
    {
      if (it->empty())
      {
        it = callbacks.erase(it);
        continue;
      }

      (*it)(event);
      ++it;
    }

    XFreeEventData(xiev.xany.display, cookie);
    invoking_callbacks_ = false;

    // A callback might unregister itself on the event callback, causing the
    // above callbacks loop to crash, so in this case we save the event in the
    // removal queue and eventually we unregistered these callbacks.
    bool update_event_monitor = false;
    for (auto it = removal_queue_.begin(); it != removal_queue_.end(); it = removal_queue_.erase(it))
    {
      auto const& cb = *it;
      pointer_callbacks_.erase(cb);
      key_callbacks_.erase(cb);
      barrier_callbacks_.erase(cb);
      update_event_monitor = true;
    }

    if (callbacks.empty() || update_event_monitor)
    {
      idle_removal_.reset(new glib::Idle([this] {
        UpdateEventMonitor();
        return false;
      }));

      return false;
    }

    return true;
  }

  int xi_opcode_;
  bool event_filter_set_;
  bool invoking_callbacks_;
  glib::Source::UniquePtr idle_removal_;
  EventCallbackSet pointer_callbacks_;
  EventCallbackSet key_callbacks_;
  EventCallbackSet barrier_callbacks_;
  EventCallbackSet removal_queue_;
};

Monitor::Monitor()
{
  if (instance_)
  {
    LOG_WARN(logger) << "More than one input::Monitor created.";
    return;
  }

  instance_ = this;
  impl_.reset(new Impl());
}

Monitor::~Monitor()
{
  if (this == instance_)
    instance_ = nullptr;
}

Monitor& Monitor::Get()
{
  if (!instance_)
  {
    LOG_ERROR(logger) << "No input::Monitor created yet.";
  }

  return *instance_;
}

bool Monitor::RegisterClient(Events events, EventCallback const& cb)
{
  return impl_->RegisterClient(events, cb);
}

bool Monitor::UnregisterClient(EventCallback const& cb)
{
  return impl_->UnregisterClient(cb);
}

Events Monitor::RegisteredEvents(EventCallback const& cb) const
{
  return impl_->RegisteredEvents(cb);
}

} // input namespace
} // unity namespace
