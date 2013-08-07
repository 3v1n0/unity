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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
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
  int const X_BREAK_BUFFER = 20;
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

  if (vertical_barriers_.size() > num_monitors)
    vertical_barriers_.resize(num_monitors);

  if (horizontal_barriers_.size() > num_monitors)
    horizontal_barriers_.resize(num_monitors);

  while (vertical_barriers_.size() < num_monitors)
  {
    auto barrier = std::make_shared<PointerBarrierWrapper>();
    barrier->orientation = VERTICAL;
    barrier->barrier_event.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnPointerBarrierEvent));
    vertical_barriers_.push_back(barrier);
  }

  while (horizontal_barriers_.size() < num_monitors)
  {
    auto barrier = std::make_shared<PointerBarrierWrapper>();
    barrier->orientation = HORIZONTAL;
    barrier->barrier_event.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnPointerBarrierEvent));
    horizontal_barriers_.push_back(barrier);
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
    auto vertical_barrier = vertical_barriers_[i];
    auto horizontal_barrier = horizontal_barriers_[i];
    auto monitor = layout[i];

    vertical_barrier->DestroyBarrier();
    horizontal_barrier->DestroyBarrier();

    if (edge_resist)
    {
      horizontal_barrier->x1 = monitor.x;
      horizontal_barrier->x2 = monitor.x + monitor.width;
      horizontal_barrier->y1 = monitor.y;
      horizontal_barrier->y2 = monitor.y;
      horizontal_barrier->index = i;
      horizontal_barrier->direction = UP;

      horizontal_barrier->threshold = parent_->options()->edge_stop_velocity();
      horizontal_barrier->max_velocity_multiplier = parent_->options()->edge_responsiveness();

      horizontal_barrier->ConstructBarrier();
    }

    if (!edge_resist && parent_->options()->hide_mode() == launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER)
      continue;

    vertical_barrier->x1 = monitor.x;
    vertical_barrier->x2 = monitor.x;
    vertical_barrier->y1 = monitor.y;
    vertical_barrier->y2 = monitor.y + monitor.height;
    vertical_barrier->index = i;

    vertical_barrier->threshold = parent_->options()->edge_stop_velocity();
    vertical_barrier->max_velocity_multiplier = parent_->options()->edge_responsiveness();

    vertical_barrier->ConstructBarrier();
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
  for (auto barrier : vertical_barriers_)
    if (barrier->OwnsBarrierEvent(barrier_event->barrier))
      return barrier;

  for (auto barrier : horizontal_barriers_)
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
  if ((owner->orientation == VERTICAL and EventIsInsideYBreakZone(event)) or
      (owner->orientation == HORIZONTAL and EventIsInsideXBreakZone(event)))
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

bool EdgeBarrierController::Impl::EventIsInsideXBreakZone(BarrierEvent::Ptr const& event)
{
  static int x_break_zone = event->y;

  if (decaymulator_.value <= 0)
    x_break_zone = event->x;

  if (event->x <= x_break_zone + X_BREAK_BUFFER &&
      event->x >= x_break_zone - X_BREAK_BUFFER)
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
  auto orientation = owner->orientation();
  auto result = EdgeBarrierSubscriber::Result::NEEDS_RELEASE;

  auto subscribers = orientation == VERTICAL ? vertical_subscribers_ : horizontal_subscribers_ ;

  if (monitor < subscribers.size())
  {
    auto subscriber = subscribers[monitor];

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

void EdgeBarrierController::AddVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->vertical_subscribers_.size())
    pimpl->vertical_subscribers_.resize(monitor + 1);

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->vertical_subscribers_[monitor] = subscriber;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

void EdgeBarrierController::RemoveVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->vertical_subscribers_.size() || pimpl->vertical_subscribers_[monitor] != subscriber)
    return;

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->vertical_subscribers_[monitor] = nullptr;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

void EdgeBarrierController::AddHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->horizontal_subscribers_.size())
    pimpl->horizontal_subscribers_.resize(monitor + 1);

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->horizontal_subscribers_[monitor] = subscriber;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

void EdgeBarrierController::RemoveHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  if (monitor >= pimpl->horizontal_subscribers_.size() || pimpl->horizontal_subscribers_[monitor] != subscriber)
    return;

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  pimpl->horizontal_subscribers_[monitor] = nullptr;
  pimpl->ResizeBarrierList(monitors);
  pimpl->SetupBarriers(monitors);
}

EdgeBarrierSubscriber* EdgeBarrierController::GetVerticalSubscriber(unsigned int monitor)
{
  if (monitor >= pimpl->vertical_subscribers_.size())
    return nullptr;

  return pimpl->vertical_subscribers_[monitor];
}

EdgeBarrierSubscriber* EdgeBarrierController::GetHorizontalSubscriber(unsigned int monitor)
{
  if (monitor >= pimpl->horizontal_subscribers_.size())
    return nullptr;

  return pimpl->horizontal_subscribers_[monitor];
}

}
}
