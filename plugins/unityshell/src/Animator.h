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

#ifndef UNITY_ANIMATOR_H_
#define UNITY_ANIMATOR_H_

#include <glib.h>
#include <cstdint>
#include <sigc++/sigc++.h>

namespace unity
{
class Animator
{
public:
  Animator(unsigned int default_duration, unsigned int fps_rate = 30);
  ~Animator();

  void SetRate(unsigned int fps_rate);
  void SetDuration(unsigned int duration);

  unsigned int GetRate();
  unsigned int GetDuration();
  double GetProgress();
  bool IsRunning();

  void Start(double start_progress = 0.0f);
  void Start(unsigned int one_time_duration, double start_progress = 0.0f);
  void Stop();

  sigc::signal<void> animation_started;
  sigc::signal<void> animation_ended;

  sigc::signal<void, double> animation_updated;
  sigc::signal<void, double> animation_stopped;

private:
  int64_t _start_time;
  unsigned int _rate;
  unsigned int _duration;
  unsigned int _one_time_duration;
  unsigned int _timeout_id;
  double _start_progress;
  double _progress;

  static gboolean TimerTimeOut(Animator *self);
};

}
#endif
