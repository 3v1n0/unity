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
 */

#include "EdgeBarrierController.h"
#include "Decaymulator.h"
#include "unity-shared/UScreen.h"

namespace unity {
namespace ui {

struct EdgeBarrierController::Impl
{
  Impl(EdgeBarrierController *parent);
  ~Impl();

  void ResizeBarrierList(std::vector<nux::Geometry> const& layout);
  void SetupBarriers(std::vector<nux::Geometry> const& layout);

  void OnPointerBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event);

  std::vector<PointerBarrierWrapper::Ptr> barriers_;
  Decaymulator::Ptr decaymulator_;
  float edge_overcome_pressure_;
  EdgeBarrierController* parent_;
  std::vector<EdgeBarrierSubscriber*> subscribers_;
};

EdgeBarrierController::Impl::Impl(EdgeBarrierController *parent)
  : decaymulator_(Decaymulator::Ptr(new Decaymulator()))
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

  parent_->sticky_edges.changed.connect([&](bool value) {
    SetupBarriers(UScreen::GetDefault()->GetMonitors());
  });

  parent_->options.changed.connect([&](launcher::Options::Ptr options) {
    options->option_changed.connect([&]() {
      SetupBarriers(UScreen::GetDefault()->GetMonitors());
    });
    SetupBarriers(UScreen::GetDefault()->GetMonitors());
  });
}

EdgeBarrierController::Impl::~Impl()
{

}

void EdgeBarrierController::Impl::ResizeBarrierList(std::vector<nux::Geometry> const& layout)
{
  size_t num_monitors = layout.size();
  if (barriers_.size() > num_monitors)
  {
    barriers_.resize(num_monitors);
  }

  while (barriers_.size() < num_monitors)
  {
    auto barrier = PointerBarrierWrapper::Ptr(new PointerBarrierWrapper());
    barrier->barrier_event.connect(sigc::mem_fun(this, &EdgeBarrierController::Impl::OnPointerBarrierEvent));
    barriers_.push_back(barrier);
  }
}

void EdgeBarrierController::Impl::SetupBarriers(std::vector<nux::Geometry> const& layout)
{
  bool edge_resist = parent_->options()->edge_resist();

  size_t size = layout.size();
  for (size_t i = 0; i < size; i++)
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
  decaymulator_->rate_of_decay = parent_->options()->edge_decay_rate() * decay_responsiveness_mult;
  
  float overcome_responsiveness_mult = ((parent_->options()->edge_responsiveness() - 1) * 1.0f) + 1;
  edge_overcome_pressure_ = parent_->options()->edge_overcome_pressure() * overcome_responsiveness_mult;
}

void EdgeBarrierController::Impl::OnPointerBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event)
{
  int monitor = owner->index;
  bool process = true;

  if ((size_t)monitor <= subscribers_.size())
  {
    auto subscriber = subscribers_[monitor];
    if (subscriber && subscriber->HandleBarrierEvent(owner, event))
      process = false;
  }

  if (process && owner->x1 > 0)
  {
    decaymulator_->value = decaymulator_->value + event->velocity;
    if (decaymulator_->value > edge_overcome_pressure_ || !parent_->options()->edge_resist())
    {
      owner->ReleaseBarrier(event->event_id);
      decaymulator_->value = 0;
    }
  }
  else
  {
    decaymulator_->value = 0;
  }
}

EdgeBarrierController::EdgeBarrierController()
  : sticky_edges(false)
  , pimpl(new Impl(this))
{
}

EdgeBarrierController::~EdgeBarrierController()
{

}

void EdgeBarrierController::Subscribe(EdgeBarrierSubscriber* subscriber, int monitor)
{
  if (pimpl->subscribers_.size() <= (size_t)monitor)
    pimpl->subscribers_.resize(monitor + 1);
  pimpl->subscribers_[monitor] = subscriber;

  pimpl->SetupBarriers(UScreen::GetDefault()->GetMonitors());
}

void EdgeBarrierController::Unsubscribe(EdgeBarrierSubscriber* subscriber, int monitor)
{
  if (pimpl->subscribers_.size() < (size_t)monitor || pimpl->subscribers_[monitor] != subscriber)
    return;
  pimpl->subscribers_[monitor] = nullptr;

  pimpl->SetupBarriers(UScreen::GetDefault()->GetMonitors());
}


}
}
