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

#include <cstdint>
#include <UnityCore/GLibSource.h>

namespace unity
{
class IdleAnimator : boost::noncopyable
{
public:
  IdleAnimator(unsigned int duration);
  ~IdleAnimator();

  void SetDuration(unsigned int duration);

  unsigned int GetRate() const;
  unsigned int GetDuration() const;
  double GetProgress() const;
  bool IsRunning() const;

  void Start(int source, int destination);

  sigc::signal<void> animation_started;
  sigc::signal<void> animation_ended;

  sigc::signal<void, double> animation_updated;
  sigc::signal<void, double> animation_stopped;

private:
  bool DoStep();
  unsigned int duration_;

    // Animation
  struct timespec times;
  glib::SourceManager sources_;
};

}
#endif
