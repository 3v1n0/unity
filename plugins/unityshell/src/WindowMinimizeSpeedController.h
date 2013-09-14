// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2010-11 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Your own copyright notice would go above. You are free to choose whatever
 * licence you want, just take note that some compiz code is GPL and you will
 * not be able to re-use it if you want to use a different licence.
 */

#ifndef WINDOWMINIMIZESPEEDCONTROLLER_H
#define WINDOWMINIMIZESPEEDCONTROLLER_H

#include <core/core.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>
#include <sigc++/sigc++.h>

typedef struct _GSettings GSettings;

using namespace unity;

class WindowMinimizeSpeedController
{
public:
  WindowMinimizeSpeedController();
  void UpdateCount();
  int getDuration();
  sigc::signal<void> DurationChanged;

private:
  void SetDuration();

  glib::Object<GSettings> _settings;
  int _minimize_count;
  int _minimize_speed_threshold;
  int _minimize_slow_duration;
  int _minimize_fast_duration;
  glib::Signal<void, GSettings*, gchar* > _minimize_count_changed;
  glib::Signal<void, GSettings*, gchar* > _minimize_speed_threshold_changed;
  glib::Signal<void, GSettings*, gchar* > _minimize_slow_duration_changed;
  glib::Signal<void, GSettings*, gchar* > _minimize_fast_duration_changed;
  int _duration;
};

#endif // WINDOWMINIMIZESPEEDCONTROLLER_H
