// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#include "IdleAnimator.h"

namespace unity
{

IdleAnimator::IdleAnimator(unsigned int duration_)
  : duration_(0)
{
  SetDuration(duration_);
}

IdleAnimator::~IdleAnimator()
{
  Stop();
}


void IdleAnimator::SetDuration(unsigned int duration)
{
  duration_ = duration * 1000;
}

unsigned int IdleAnimator::GetRate() const
{
  if (rate_ != 0)
    return 1000 / rate_;

  return rate_;
}

unsigned int IdleAnimator::GetDuration() const
{
  return (one_time_duration_ > 0 ? one_time_duration_ : duration_) / 1000;
}

bool IdleAnimator::IsRunning() const
{
  // short circuit to avoid unneeded calculations
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  // hover in animation
  if (unity::TimeUtil::TimeDelta(&current, &_times) < ANIM_DURATION_LONG)
    return true;

  return false;
}

void IdleAnimator::Start(unsigned int one_time_duration, double start_progress)
{
  DoStep();
}

void IdleAnimator::Start(double start_progress)
{
  Start(0, start_progress);
}

void IdleAnimator::Stop()
{
  if (timeout_)
  {
    timeout_.reset();
    animation_updated.emit(progress_);
    animation_ended.emit();
    animation_stopped.emit(progress_);
    one_time_duration_ = 0;
  }
}

bool IdleAnimator::DoStep()
{
  // rely on the compiz event loop to come back to us in a nice throttling
  if (IsRunning())
  {
    auto idle = std::make_shared<glib::Idle>(glib::Source::Priority::DEFAULT);
    sources_.Add(idle, ANIMATION_IDLE);
    idle->Run([&]() {
      EnsureAnimation();
      return false;
    });
  }
}

} //namespace
