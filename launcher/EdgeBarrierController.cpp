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
#include "Decaymulator.h"
#include "unity-shared/UScreen.h"
#include "UnityCore/GLibSource.h"

namespace unity {
namespace ui {

struct EdgeBarrierController::Impl
{
  Impl(EdgeBarrierController *parent);

  void ResizeBarrierList(std::vector<nux::Geometry> const& layout);
  void SetupBarriers(std::vector<nux::Geometry> const& layout);

  void OnPointerBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event);
  void BarrierRelease(PointerBarrierWrapper* owner, int event);

  bool StickyEdgeSetter(bool const& new_val)
  {
    if (parent_->options() && new_val != parent_->options()->edge_resist())
    {
      parent_->options()->edge_resist = new_val;
      return true;
    }
    return false;
  }

  bool StickyEdgeGetter()
  {
    return parent_->options() ? parent_->options()->edge_resist() : false;
  }

  std::vector<PointerBarrierWrapper::Ptr> barriers_;
  std::vector<EdgeBarrierSubscriber*> subscribers_;
  Decaymulator decaymulator_;
  glib::Source::UniquePtr release_timeout_;
  float edge_overcome_pressure_;
  EdgeBarrierController* parent_;
};

EdgeBarrierController::Impl::Impl(EdgeBarrierController *parent)
  : edge_overcome_pressure_(0)
  , parent_(parent)
{
  UScreen *uscreen = UScreen::GetDefault();

  auto monitors = uscreen->GetMonitors();
  ResizeBarrierList(monitors);

  uscreen->changed.connect([&](int primary, std::vector<nux::Geometry>& layout) {
    ResizeBarrierList(layout);
    SetupBarriers(layout);
  });

  parent_->sticky_edges.SetGetterFunction(sigc::mem_fun(this, &Impl::StickyEdgeGetter));
  parent_->sticky_edges.SetSetterFunction(sigc::mem_fun(this, &Impl::StickyEdgeSetter));

/* Set this back, once lp:~3v1n0/nux/use-std-function is merged
  parent_->sticky_edges.SetGetterFunction([parent_] {
    return parent_->options() ? parent_->options()->edge_resist() : false;
  });

  parent_->sticky_edges.SetSetterFunction([parent_] (bool const& new_val) {
    if (parent_->options() && new_val != parent_->options()->edge_resist())
    {
      parent_->options()->edge_resist = new_val;
      return true;
    }
    return false;
  });
  */

  parent_->options.changed.connect([&](launcher::Options::Ptr options) {
    options->option_changed.connect([&]() {
      SetupBarriers(UScreen::GetDefault()->GetMonitors());
    });
    SetupBarriers(UScreen::GetDefault()->GetMonitors());
  });
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

void EdgeBarrierController::Impl::SetupBarriers(std::vector<nux::Geometry> const& layout)
{
  bool edge_resist = parent_->sticky_edges();

  for (unsigned i = 0; i < layout.size(); i++)
  {
    auto barrier = barriers_[i];
    auto monitor = layout[i];

    barrier->DestroyBarrier();

    if (!edge_resist && (subscribers_[i] == nullptr || parent_->options()->hide_mode() == launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER))
      continue;

    barrier->x1 = monitor.x;
    barrier->x2 = monitor.x;
    barrier->y1 = monitor.y;
    barrier->y2 = monitor.y + monitor.height;
    barrier->index = i;

    barrier->threshold = parent_->options()->edge_stop_velocity();
    barrier->max_velocity_multiplier = parent_->options()->edge_responsiveness();

    if (edge_resist)
      barrier->direction = BarrierDirection::BOTH;
    else
      barrier->direction = BarrierDirection::LEFT;

    barrier->ConstructBarrier();
  }

  float decay_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * .3f) + 1;
  decaymulator_.rate_of_decay = parent_->options()->edge_decay_rate() * decay_responsiveness_mult;

  float overcome_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * 1.0f) + 1;
  edge_overcome_pressure_ = parent_->options()->edge_overcome_pressure() * overcome_responsiveness_mult;
}

void EdgeBarrierController::Impl::OnPointerBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
{
  unsigned int monitor = owner->index;
  bool process = true;

  if (monitor < subscribers_.size())
  {
    auto subscriber = subscribers_[monitor];

    if (subscriber && subscriber->HandleBarrierEvent(owner, event))
      process = false;
  }

  if (process && owner->released)
  {
    BarrierRelease(owner, event->event_id);
  }
  else if (process && owner->x1 > 0)
  {
    decaymulator_.value = decaymulator_.value + event->velocity;

    if (decaymulator_.value > edge_overcome_pressure_ || (!parent_->sticky_edges() && !subscribers_[monitor]))
    {
      BarrierRelease(owner, event->event_id);
    }
  }
  else
  {
    decaymulator_.value = 0;
  }
}

void EdgeBarrierController::Impl::BarrierRelease(PointerBarrierWrapper* owner, int event)
{
  owner->ReleaseBarrier(event);
  owner->released = true;
  decaymulator_.value = 0;

  unsigned duration = parent_->options()->edge_passed_disabled_ms;
  release_timeout_.reset(new glib::Timeout(duration, [owner] {
    owner->released = false;
    return false;
  }));
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

void EdgeBarrierController::ProcessBarrierEvent(PointerBarrierWrapper* owner, BarrierEvent::Ptr event)
{
  pimpl->OnPointerBarrierEvent(owner, event);
}

}
}
