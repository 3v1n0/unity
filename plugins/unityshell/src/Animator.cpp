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
  _start_time = 0;
  _timeout_id = 0;
  _progress = 0.0f;
  _start_progress = 0.0f;
  _rate = 1;
  _duration = 0;
  _one_time_duration = 0;

  SetDuration(default_duration);
  SetRate(fps_rate);
}

Animator::~Animator()
{
  if (_timeout_id != 0)
    g_source_remove (_timeout_id);
}

void
Animator::SetRate(unsigned int fps_rate)
{
  if (fps_rate != 0)
    _rate = 1000 / fps_rate;
}

void
Animator::SetDuration(unsigned int duration)
{
  _duration = duration * 1000;
}

unsigned int
Animator::GetRate()
{
  return _rate;
}

unsigned int
Animator::GetDuration()
{
  return (_one_time_duration > 0 ? _one_time_duration : _duration) / 1000;
}

bool
Animator::IsRunning()
{
  return (_timeout_id != 0);
}

double
Animator::GetProgress()
{
  return _progress;
}

void
Animator::Start(unsigned int one_time_duration, double start_progress)
{
  if (_timeout_id == 0 && start_progress < 1.0f)
  {
    if (start_progress < 0.0f)
      start_progress = 0.0f;

    _one_time_duration = one_time_duration * 1000;
    _start_progress = start_progress;
    _progress = _start_progress;
    _start_time = g_get_monotonic_time();
    _timeout_id = g_timeout_add(_rate, (GSourceFunc) &Animator::TimerTimeOut, this);
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
  if (_timeout_id != 0)
  {
    g_source_remove(_timeout_id);
    animation_updated.emit(_progress);
    animation_ended.emit();
    animation_stopped.emit(_progress);
    _one_time_duration = 0;
    _timeout_id = 0;
  }
}

gboolean
Animator::TimerTimeOut(Animator *self)
{
  const gint64 current_time = g_get_monotonic_time();
  const gint64 duration = self->_one_time_duration > 0 ? self->_one_time_duration : self->_duration;
  const gint64 end_time = self->_start_time + duration;

  if (current_time < end_time && self->_progress < 1.0f && duration > 0)
  {
    const double diff_time = current_time - self->_start_time;
    self->_progress = CLAMP(self->_start_progress + (diff_time / duration), 0.0f, 1.0f);
    self->animation_updated.emit(self->_progress);

    return TRUE;
  }
  else
  {
    self->_progress = 1.0f;
    self->animation_updated.emit(1.0f);
    self->animation_ended.emit();
    self->_one_time_duration = 0;
    self->_timeout_id = 0;

    return FALSE;
  }
}

} //namespace
