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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <X11/extensions/XInput2.h>
#include <UnityCore/GLibSource.h>
#include <unordered_set>
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
const unsigned XINPUT_MINOR_VERSION = 2;

bool operator&(Events l, Events r)
{
  typedef std::underlying_type<Events>::type ut;
  return static_cast<ut>(static_cast<ut>(l) & static_cast<ut>(r));
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
  ev->x_root = std::round(xiev->root_y);
  ev->y_root = std::round(xiev->root_x);
  ev->state = xiev->mods.effective;
  ev->same_screen = True;
}

template <typename EVENT_TYPE>
void initialize_event(XEvent* ev, XIDeviceEvent* xiev);

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
}

struct Monitor::Impl
{
  Impl()
    : xi_opcode_(0)
    , event_filter_set_(false)
    , invoking_callbacks_(false)
  {
    Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
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
      UpdateEventMonitor();
    }
  }

  void RegisterClient(Events type, EventCallback const& cb)
  {
    if (type & Events::POINTER)
      pointer_callbacks_.insert(cb);

    if (type & Events::KEYS)
      key_callbacks_.insert(cb);

    if (!pointer_callbacks_.empty() || !key_callbacks_.empty())
      UpdateEventMonitor();
  }

  void UnregisterClient(EventCallback const& cb)
  {
    if (invoking_callbacks_)
    {
      // Delay the event removal if we're currently invoking a callback
      // not to break the callbacks loop
      removal_queue_.insert(cb);
      return;
    }

    pointer_callbacks_.erase(cb);
    key_callbacks_.erase(cb);
    UpdateEventMonitor();
  }

  void UpdateEventMonitor()
  {
    auto* dpy = nux::GetGraphicsDisplay()->GetX11Display();
    Window root = DefaultRootWindow(dpy);
    XIEventMask* mask = nullptr;

    int masks_size = 0;
    std::unique_ptr<XIEventMask[], int(*)(void*)> selected(XIGetSelectedEvents(dpy, root, &masks_size), XFree);

    for (int i = 0; i < masks_size; ++i)
    {
      if (selected[i].deviceid == XIAllDevices)
      {
        mask = &(selected[i]);
        break;
      }
    }

    if (!mask)
    {
      ++masks_size;
      auto* new_selected = g_new(XIEventMask, masks_size);
      g_memmove(new_selected, selected.get(), sizeof(XIEventMask) * (masks_size - 1));
      selected.reset(new_selected);
      mask = &(selected[masks_size-1]);
    }

    unsigned char mask_bits[XIMaskLen(XI_LASTEVENT)] = { 0 };
    *mask = { XIAllDevices, sizeof(mask_bits), mask_bits };

    if (!pointer_callbacks_.empty())
    {
      XISetMask(mask->mask, XI_Motion);
      XISetMask(mask->mask, XI_ButtonPress);
      XISetMask(mask->mask, XI_ButtonRelease);
    }

    if (!key_callbacks_.empty())
    {
      XISetMask(mask->mask, XI_KeyPress);
      XISetMask(mask->mask, XI_KeyRelease);
    }

    XISelectEvents(dpy, root, selected.get(), masks_size);
    XSync(dpy, False);

    if (!pointer_callbacks_.empty() || !key_callbacks_.empty())
    {
      if (!event_filter_set_)
      {
        nux::GetGraphicsDisplay()->AddEventFilter({[] (XEvent event, void* data) {
          return static_cast<Impl*>(data)->HandleEvent(event);
        }, this});

        event_filter_set_ = true;
      }
    }
    else if (event_filter_set_)
    {
      nux::GetGraphicsDisplay()->RemoveEventFilter(this);
      event_filter_set_ = false;
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
    }

    return handled;
  }

  template <typename EVENT_TYPE>
  bool InvokeCallbacks(std::unordered_set<EventCallback>& callbacks, XEvent& xiev)
  {
    XGenericEventCookie *cookie = &xiev.xcookie;

    if (!XGetEventData(xiev.xany.display, cookie))
      return false;

    XEvent event;
    initialize_event<EVENT_TYPE>(&event, reinterpret_cast<XIDeviceEvent*>(cookie->data));
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
  std::unordered_set<EventCallback> pointer_callbacks_;
  std::unordered_set<EventCallback> key_callbacks_;
  std::unordered_set<EventCallback> removal_queue_;
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

void Monitor::RegisterClient(Events events, EventCallback const& cb)
{
  impl_->RegisterClient(events, cb);
}

void Monitor::UnregisterClient(EventCallback const& cb)
{
  impl_->UnregisterClient(cb);
}

} // input namespace
} // unity namespace
