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
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/InputMonitor.h"
#include "UnityCore/GLibSource.h"

namespace unity
{
namespace ui
{

namespace
{
  int const Y_BREAK_BUFFER = 20;
  int const X_BREAK_BUFFER = 20;
}

EdgeBarrierController::Impl::Impl(EdgeBarrierController *parent)
  : edge_overcome_pressure_(0)
  , parent_(parent)
{
  UScreen *uscreen = UScreen::GetDefault();

  auto monitors = uscreen->GetMonitors();
  ResizeBarrierList(monitors);

  /* FIXME: Back to c++11 lambda once we get sigc::track_obj.
  uscreen->changed.connect(sigc::track_obj(([this](int primary, std::vector<nux::Geometry> const& layout) {
    ResizeBarrierList(layout);
    SetupBarriers(layout);
  }));*/

  uscreen->changed.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnUScreenChanged));
  Settings::Instance().launcher_position.changed.connect(sigc::hide(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnOptionsChanged)));

  parent_->force_disable.changed.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnForceDisableChanged));

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

  parent_->options.changed.connect([this](launcher::Options::Ptr options) {
    /* FIXME: Back to c++11 lambda once we get sigc::track_obj.
    options->option_changed.connect([this]() {
      SetupBarriers(UScreen::GetDefault()->GetMonitors());
    });*/
    options->option_changed.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnOptionsChanged));
    SetupBarriers(UScreen::GetDefault()->GetMonitors());
  });
}

EdgeBarrierController::Impl::~Impl()
{
  nux::GetGraphicsDisplay()->RemoveEventFilter(this);
}

void EdgeBarrierController::Impl::OnUScreenChanged(int primary, std::vector<nux::Geometry> const& layout)
{
  ResizeBarrierList(layout);
  SetupBarriers(layout);
}

void EdgeBarrierController::Impl::OnForceDisableChanged(bool value)
{
  auto monitors = UScreen::GetDefault()->GetMonitors();
  ResizeBarrierList(monitors);
  SetupBarriers(monitors);
}

void EdgeBarrierController::Impl::OnOptionsChanged()
{
  SetupBarriers(UScreen::GetDefault()->GetMonitors());
}

void EdgeBarrierController::Impl::AddSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor, std::vector<EdgeBarrierSubscriber*>& subscribers)
{
  if (monitor >= subscribers.size())
    subscribers.resize(monitor + 1);

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  subscribers[monitor] = subscriber;
  ResizeBarrierList(monitors);
  SetupBarriers(monitors);
}

void EdgeBarrierController::Impl::RemoveSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor, std::vector<EdgeBarrierSubscriber*>& subscribers)
{
  if (monitor >= subscribers.size() || subscribers[monitor] != subscriber)
    return;

  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  subscribers[monitor] = nullptr;
  ResizeBarrierList(monitors);
  SetupBarriers(monitors);
}

void EdgeBarrierController::Impl::ResizeBarrierList(std::vector<nux::Geometry> const& layout)
{
  if (parent_->force_disable)
  {
    vertical_barriers_.clear();
    horizontal_barriers_.clear();
    return;
  }

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

void EdgeBarrierController::Impl::SetupBarriers(std::vector<nux::Geometry> const& layout)
{
  if (parent_->force_disable())
    return;

  size_t monitors_size = layout.size();
  auto launcher_position = Settings::Instance().launcher_position();
  bool edge_resist = parent_->sticky_edges();
  bool needs_barrier = edge_resist && monitors_size > 1;
  bool needs_vertical_barrier = needs_barrier;

  if (parent_->options()->hide_mode() != launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER)
    needs_vertical_barrier = true;

  for (unsigned i = 0; i < layout.size(); ++i)
  {
    auto const& vertical_barrier = vertical_barriers_[i];
    auto const& horizontal_barrier = horizontal_barriers_[i];
    auto const& monitor = layout[i];

    vertical_barrier->DestroyBarrier();
    horizontal_barrier->DestroyBarrier();

    if (needs_barrier)
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

    if (!needs_vertical_barrier)
      continue;

    if (launcher_position == LauncherPosition::LEFT)
    {
      vertical_barrier->x1 = monitor.x;
      vertical_barrier->x2 = monitor.x;
      vertical_barrier->y1 = monitor.y;
      vertical_barrier->y2 = monitor.y + monitor.height;
    }
    else
    {
      vertical_barrier->x1 = monitor.x;
      vertical_barrier->x2 = monitor.x + monitor.width;
      vertical_barrier->y1 = monitor.y + monitor.height;
      vertical_barrier->y2 = monitor.y + monitor.height;
      vertical_barrier->direction = DOWN;
    }

    vertical_barrier->index = i;

    vertical_barrier->threshold = parent_->options()->edge_stop_velocity();
    vertical_barrier->max_velocity_multiplier = parent_->options()->edge_responsiveness();

    vertical_barrier->ConstructBarrier();
  }

  if (needs_barrier || needs_vertical_barrier)
    input::Monitor::Get().RegisterClient(input::Events::BARRIER, sigc::mem_fun(this, &Impl::HandleEvent));
  else
    input::Monitor::Get().UnregisterClient(sigc::mem_fun(this, &Impl::HandleEvent));

  float decay_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * .3f) + 1;
  decaymulator_.rate_of_decay = parent_->options()->edge_decay_rate() * decay_responsiveness_mult;

  float overcome_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * 1.0f) + 1;
  edge_overcome_pressure_ = parent_->options()->edge_overcome_pressure() * overcome_responsiveness_mult;
}

void EdgeBarrierController::Impl::HandleEvent(XEvent const& xevent)
{
  if (xevent.xcookie.evtype != XI_BarrierHit)
    return;

  auto* barrier_event = reinterpret_cast<XIBarrierEvent*>(xevent.xcookie.data);
  PointerBarrierWrapper::Ptr const& wrapper = FindBarrierEventOwner(barrier_event);

  if (wrapper)
    wrapper->HandleBarrierEvent(barrier_event);
}

PointerBarrierWrapper::Ptr EdgeBarrierController::Impl::FindBarrierEventOwner(XIBarrierEvent* barrier_event)
{
  for (auto const& barrier : vertical_barriers_)
    if (barrier->OwnsBarrierEvent(barrier_event->barrier))
      return barrier;

  for (auto const& barrier : horizontal_barriers_)
    if (barrier->OwnsBarrierEvent(barrier_event->barrier))
      return barrier;

  return nullptr;
}

void EdgeBarrierController::Impl::BarrierReset()
{
  decaymulator_.value = 0;
}

void EdgeBarrierController::Impl::BarrierPush(PointerBarrierWrapper::Ptr const& owner, BarrierEvent::Ptr const& event)
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

void EdgeBarrierController::Impl::OnPointerBarrierEvent(PointerBarrierWrapper::Ptr const& owner, BarrierEvent::Ptr const& event)
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

void EdgeBarrierController::Impl::BarrierRelease(PointerBarrierWrapper::Ptr const& owner, int event)
{
  owner->ReleaseBarrier(event);
  owner->released = true;
  BarrierReset();

  if (!owner->release_once() ||
      (owner->release_once() && (!release_timeout_ || !release_timeout_->IsRunning())))
  {
    unsigned duration = parent_->options()->edge_passed_disabled_ms;

    std::weak_ptr<PointerBarrierWrapper> owner_weak(owner);
    release_timeout_.reset(new glib::Timeout(duration, [owner_weak] {
      if (PointerBarrierWrapper::Ptr const& owner = owner_weak.lock())
      {
        owner->released = false;
        owner->release_once = false;
      }
      return false;
    }));
  }
}

EdgeBarrierController::EdgeBarrierController()
  : force_disable(false)
  , pimpl(new Impl(this))
{}

EdgeBarrierController::~EdgeBarrierController()
{}

void EdgeBarrierController::AddVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  pimpl->AddSubscriber(subscriber, monitor, pimpl->vertical_subscribers_);
}

void EdgeBarrierController::RemoveVerticalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  pimpl->RemoveSubscriber(subscriber, monitor, pimpl->vertical_subscribers_);
}

void EdgeBarrierController::AddHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  pimpl->AddSubscriber(subscriber, monitor, pimpl->horizontal_subscribers_);
}

void EdgeBarrierController::RemoveHorizontalSubscriber(EdgeBarrierSubscriber* subscriber, unsigned int monitor)
{
  pimpl->RemoveSubscriber(subscriber, monitor, pimpl->horizontal_subscribers_);
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
