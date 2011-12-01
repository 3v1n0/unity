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

#include "Animator.h"

namespace unity
{

Animator::Animator(unsigned int default_duration, unsigned int fps_rate)
{
  start_time_ = 0;
  timeout_id_ = 0;
  progress_ = 0.0f;
  start_progress_ = 0.0f;
  rate_ = 1;
  duration_ = 0;
  one_time_duration_ = 0;

  SetDuration(default_duration);
  SetRate(fps_rate);
}

Animator::~Animator()
{
  if (timeout_id_ != 0)
    g_source_remove (timeout_id_);
}

void
Animator::SetRate(unsigned int fps_rate)
{
  if (fps_rate != 0)
    rate_ = 1000 / fps_rate;
}

void
Animator::SetDuration(unsigned int duration)
{
  duration_ = duration * 1000;
}

unsigned int
Animator::GetRate() const
{
  if (rate_ != 0)
    return 1000 / rate_;

  return rate_;
}

unsigned int
Animator::GetDuration() const
{
  return (one_time_duration_ > 0 ? one_time_duration_ : duration_) / 1000;
}

bool
Animator::IsRunning() const
{
  return (timeout_id_ != 0);
}

double
Animator::GetProgress() const
{
  return progress_;
}

void
Animator::Start(unsigned int one_time_duration, double start_progress)
{
  if (timeout_id_ == 0 && start_progress < 1.0f)
  {
    if (start_progress < 0.0f)
      start_progress = 0.0f;

    one_time_duration_ = one_time_duration * 1000;
    start_progress_ = start_progress;
    progress_ = start_progress_;
    start_time_ = g_get_monotonic_time();
    timeout_id_ = g_timeout_add(rate_, (GSourceFunc) &Animator::TimerTimeOut, this);
  }
}

void
Animator::Start(double start_progress)
{
  Start(0, start_progress);
}

void
Animator::Stop()
{
  if (timeout_id_ != 0)
  {
    g_source_remove(timeout_id_);
    animation_updated.emit(progress_);
    animation_ended.emit();
    animation_stopped.emit(progress_);
    one_time_duration_ = 0;
    timeout_id_ = 0;
  }
}

bool
Animator::DoStep()
{
  const gint64 current_time = g_get_monotonic_time();
  const gint64 duration = one_time_duration_ > 0 ? one_time_duration_ : duration_;
  const gint64 end_time = start_time_ + duration;

  if (current_time < end_time && progress_ < 1.0f && duration > 0)
  {
    const double diff_time = current_time - start_time_;
    progress_ = CLAMP(start_progress_ + (diff_time / duration), 0.0f, 1.0f);
    animation_updated.emit(progress_);

    return true;
  }
  else
  {
    progress_ = 1.0f;
    animation_updated.emit(1.0f);
    animation_ended.emit();
    one_time_duration_ = 0;
    timeout_id_ = 0;

    return false;
  }
}

gboolean
Animator::TimerTimeOut(Animator *self)
{
  return self->DoStep() ? TRUE : FALSE;
}

} //namespace
