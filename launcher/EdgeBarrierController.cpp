// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "EdgeBarrierController.h"
#include "EdgeBarrierControllerPrivate.h"
#include "Decaymulator.h"
#include <NuxCore/Logger.h>
#include "unity-shared/UScreen.h"
#include "UnityCore/GLibSource.h"

namespace unity
{
namespace ui
{

namespace
{
  int const Y_BREAK_BUFFER = 20;
  int const MAJOR = 2;
  int const MINOR = 3;
}

DECLARE_LOGGER(logger, "unity.edge_barrier_controller");

int GetXI2OpCode()
{
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();

  int opcode, event_base, error_base;
  if (!XQueryExtension(dpy, "XFIXES",
                       &opcode,
                       &event_base,
                       &error_base))
  {
    LOG_ERROR(logger) << "Missing XFixes";
    return -1;
  }

  if (!XQueryExtension (dpy, "XInputExtension",
                        &opcode,
                        &event_base,
                        &error_base))
  {
    LOG_ERROR(logger) << "Missing XInput";
    return -1;
  }

  int maj = MAJOR;
  int min = MINOR;

  if (XIQueryVersion(dpy, &maj, &min) == BadRequest)
  {
    LOG_ERROR(logger) << "Need XInput version 2.3";
    return -1;
  }

  return opcode;
}

EdgeBarrierController::Impl::Impl(EdgeBarrierController *parent)
  : xi2_opcode_(-1)
  , edge_overcome_pressure_(0)
  , parent_(parent)
{
  UScreen *uscreen = UScreen::GetDefault();

  auto monitors = uscreen->GetMonitors();
  ResizeBarrierList(monitors);

  uscreen->changed.connect([&](int primary, std::vector<nux::Geometry>& layout) {
    ResizeBarrierList(layout);
    SetupBarriers(layout);
  });

  parent_->sticky_edges.SetGetterFunction([this] {
    return parent_->options() ? parent_->options()->edge_resist() : false;
  });

  parent_->sticky_edges.SetSetterFunction([this] (bool const& new_val) {
    if (parent_->options() && new_val != parent_->options()->edge_resist())
    {
      parent_->options()->edge_resist = new_val;
      return true;
    }
    return false;
  });

  parent_->options.changed.connect([&](launcher::Options::Ptr options) {
    options->option_changed.connect([&]() {
      SetupBarriers(UScreen::GetDefault()->GetMonitors());
    });
    SetupBarriers(UScreen::GetDefault()->GetMonitors());
  });

  xi2_opcode_ = GetXI2OpCode();
}

EdgeBarrierController::Impl::~Impl()
{
  nux::GetGraphicsDisplay()->RemoveEventFilter(this);
}

void EdgeBarrierController::Impl::ResizeBarrierList(std::vector<nux::Geometry> const& layout)
{
  auto num_monitors = layout.size();
  if (barriers_.size() > num_monitors)
  {
    barriers_.resize(num_monitors);
  }

  while (barriers_.size() < num_monitors)
  {
    auto barrier = std::make_shared<PointerBarrierWrapper>();
    barrier->barrier_event.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnPointerBarrierEvent));
    barriers_.push_back(barrier);
  }
}

void SetupXI2Events()
{
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();

  unsigned char mask_bits[XIMaskLen (XI_LASTEVENT)] = { 0 };
  XIEventMask mask = { XIAllMasterDevices, sizeof (mask_bits), mask_bits };

  XISetMask(mask.mask, XI_BarrierHit);
  XISetMask(mask.mask, XI_BarrierLeave);
  XISelectEvents (dpy, DefaultRootWindow(dpy), &mask, 1);
}

void EdgeBarrierController::Impl::SetupBarriers(std::vector<nux::Geometry> const& layout)
{
  bool edge_resist = parent_->sticky_edges();

  for (unsigned i = 0; i < layout.size(); i++)
  {
    auto barrier = barriers_[i];
    auto monitor = layout[i];

    barrier->DestroyBarrier();

    if (!edge_resist && parent_->options()->hide_mode() == launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER)
      continue;

    barrier->x1 = monitor.x;
    barrier->x2 = monitor.x;
    barrier->y1 = monitor.y;
    barrier->y2 = monitor.y + monitor.height;
    barrier->index = i;

    barrier->threshold = parent_->options()->edge_stop_velocity();
    barrier->max_velocity_multiplier = parent_->options()->edge_responsiveness();

    barrier->ConstructBarrier();
  }

  SetupXI2Events();
  AddEventFilter();

  float decay_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * .3f) + 1;
  decaymulator_.rate_of_decay = parent_->options()->edge_decay_rate() * decay_responsiveness_mult;

  float overcome_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * 1.0f) + 1;
  edge_overcome_pressure_ = parent_->options()->edge_overcome_pressure() * overcome_responsiveness_mult;
}

void EdgeBarrierController::Impl::AddEventFilter()
{
  // Remove an old one, if it exists
  nux::GetGraphicsDisplay()->RemoveEventFilter(this);

  nux::GraphicsDisplay::EventFilterArg event_filter;
  event_filter.filter = &HandleEventCB;
  event_filter.data = this;

  nux::GetGraphicsDisplay()->AddEventFilter(event_filter);
}

bool EdgeBarrierController::Impl::HandleEvent(XEvent xevent)
{
  Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();
  XGenericEventCookie *cookie = &xevent.xcookie;
  bool ret = false;

  switch (cookie->evtype)
  {
    case (XI_BarrierHit):
    {
      if (XGetEventData(dpy, cookie))
      {
        XIBarrierEvent* barrier_event = (XIBarrierEvent*)cookie->data;
        PointerBarrierWrapper::Ptr wrapper = FindBarrierEventOwner(barrier_event);

        if (wrapper)
          ret = wrapper->HandleBarrierEvent(barrier_event);
      }

      XFreeEventData(dpy, cookie);
      break;
    }
    default:
      break;
  }

  return ret;
}

bool EdgeBarrierController::Impl::HandleEventCB(XEvent xevent, void* data)
{
  auto edge_barrier_controller = static_cast<EdgeBarrierController::Impl*>(data);
  int const xi2_opcode = edge_barrier_controller->xi2_opcode_;

  if (xevent.type != GenericEvent || xevent.xcookie.extension != xi2_opcode)
    return false;

  return edge_barrier_controller->HandleEvent(xevent);
}

PointerBarrierWrapper::Ptr EdgeBarrierController::Impl::FindBarrierEventOwner(XIBarrierEvent* barrier_event)
{
  for (auto barrier : barriers_)
    if (barrier->OwnsBarrierEvent(barrier_event->barrier))
      return barrier;

  return nullptr;
}

void EdgeBarrierController::Impl::BarrierReset()
{
  decaymulator_.value = 0;
}

void EdgeBarrierController::Impl::BarrierPush(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event)
{
  if (EventIsInsideYBreakZone(event))
  {
    decaymulator_.value = decaymulator_.value + event->velocity;
  }
  else
  {
    BarrierReset();
  }

  if (decaymulator_.value > edge_overcome_pressure_)
  {
    BarrierRelease(owner, event->event_id);
  }
}

bool EdgeBarrierController::Impl::EventIsInsideYBreakZone(BarrierEvent::Ptr const& event)
{
  static int y_break_zone = event->y;

  if (decaymulator_.value <= 0)
    y_break_zone = event->y;

  if (event->y <= y_break_zone + Y_BREAK_BUFFER &&
      event->y >= y_break_zone - Y_BREAK_BUFFER)
  {
    return true;
  }

  return false;
}

void EdgeBarrierController::Impl::OnPointerBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr const& event)
{
  if (owner->released)
  {
    BarrierRelease(owner, event->event_id);
    return;
  }

  unsigned int monitor = owner->index;
  auto result = EdgeBarrierSubscriber::Result::NEEDS_RELEASE;

  if (monitor < subscribers_.size())
  {
    auto subscriber = subscribers_[monitor];

    if (subscriber)
      result = subscriber->HandleBarrierEvent(owner, event);
  }

  switch (result)
  {
    case EdgeBarrierSubscriber::Result::HANDLED:
      BarrierReset();
      break;

    case EdgeBarrierSubscriber::Result::ALREADY_HANDLED:
      BarrierPush(owner, event);
      break;

    case EdgeBarrierSubscriber::Result::IGNORED:
      if (parent_->sticky_edges())
      {
        BarrierPush(owner, event);
      }
      else
      {
        owner->release_once = true;
        BarrierRelease(owner, event->event_id);
      }
      break;

    case EdgeBarrierSubscriber::Result::NEEDS_RELEASE:
      BarrierRelease(owner, event->event_id);
      break;
  }
}

void EdgeBarrierController::Impl::BarrierRelease(PointerBarrierWrapper* owner, int event)
{
  owner->ReleaseBarrier(event);
  owner->released = true;
  BarrierReset();

  if (!owner->release_once() ||
      (owner->release_once() && (!release_timeout_ || !release_timeout_->IsRunning())))
  {
    unsigned duration = parent_->options()->edge_passed_disabled_ms;
    release_timeout_.reset(new glib::Timeout(duration, [owner] {
      owner->released = false;
      owner->release_once = false;
      return false;
    }));
  }
}

EdgeBarrierController::EdgeBarrierController()
  : pimpl(new Impl(this))
{}

EdgeBarrierController::~EdgeBarrierController()
{}

void EdgeBarrierController::Subscribe(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->subscribers_.size())
    pimpl->subscribers_.resize(monitor + 1);

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->subscribers_[monitor] = subscriber;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

void EdgeBarrierController::Unsubscribe(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->subscribers_.size() || pimpl->subscribers_[monitor] != subscriber)
    return;

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->subscribers_[monitor] = nullptr;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

EdgeBarrierSubscriber* EdgeBarrierController::GetSubscriber(unsigned int monitor)
{
  if (monitor >= pimpl->subscribers_.size())
    return nullptr;

  return pimpl->subscribers_[monitor];
}

}
}
