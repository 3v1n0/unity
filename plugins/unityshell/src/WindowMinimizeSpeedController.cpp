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

#include <gio/gio.h>
#include <NuxCore/Logger.h>

#include "WindowMinimizeSpeedController.h"

DECLARE_LOGGER(logger, "unity.shell.compiz.minimizer");
namespace
{
namespace local
{
const std::string UNITY_SCHEMA = "com.canonical.Unity";
}
}

WindowMinimizeSpeedController::WindowMinimizeSpeedController()
  : _settings(g_settings_new(local::UNITY_SCHEMA.c_str()))
  , _minimize_count(g_settings_get_int(_settings, "minimize-count"))
  , _minimize_speed_threshold(g_settings_get_int(_settings, "minimize-speed-threshold"))
  , _minimize_slow_duration(g_settings_get_int(_settings, "minimize-slow-duration"))
  , _minimize_fast_duration(g_settings_get_int(_settings, "minimize-fast-duration"))
  , _duration(200) // going to be overridden anyway, but at least it is initialised
{
  _minimize_count_changed.Connect(_settings, "changed::minimize-count",
                                  [&] (GSettings*, gchar* name) {
    _minimize_count = g_settings_get_int(_settings, name);
    SetDuration();
  });
  _minimize_speed_threshold_changed.Connect(_settings, "changed::minimize-speed-threshold",
                                            [&] (GSettings*, gchar* name) {
    _minimize_speed_threshold = g_settings_get_int(_settings, name);
    SetDuration();
  });
  _minimize_fast_duration_changed.Connect(_settings, "changed::minimize-fast-duration",
                                      [&] (GSettings*, gchar* name) {
    _minimize_fast_duration = g_settings_get_int(_settings, name);
    SetDuration();
  });
  _minimize_slow_duration_changed.Connect(_settings, "changed::minimize-slow-duration",
                                      [&] (GSettings*, gchar* name) {
    _minimize_slow_duration = g_settings_get_int(_settings, name);
    SetDuration();
  });
}

void WindowMinimizeSpeedController::UpdateCount()
{
  if (_minimize_count < _minimize_speed_threshold) {
    _minimize_count += 1;
    g_settings_set_int(_settings, "minimize-count", _minimize_count);
  }
}

int WindowMinimizeSpeedController::getDuration()
{
  return _duration;
}

void WindowMinimizeSpeedController::SetDuration()
{
  /* Perform some sanity checks on the configuration values */
  if (_minimize_fast_duration > _minimize_slow_duration)
  {
    LOG_WARN(logger) << "Configuration mismatch: minimize-fast-duration ("
                      << _minimize_fast_duration
                      << ") is longer than minimize-slow-duration ("
                      << _minimize_slow_duration << "). Not changing speed.";
    return;
  }

  if (_minimize_count < 0)
    _minimize_count = 0;
  if (_minimize_count > _minimize_speed_threshold)
    _minimize_count = _minimize_speed_threshold;

  /* Adjust the speed so that it gets linearly closer to maximum speed as we
     approach the threshold */
  int speed_range = _minimize_slow_duration - _minimize_fast_duration;
  float position = (_minimize_speed_threshold <= 0) ? 1.0 :
                   static_cast<float>(_minimize_count) / _minimize_speed_threshold;
  int duration = _minimize_slow_duration - std::ceil(position * speed_range);

  if (duration != _duration) {
    _duration = duration;
    DurationChanged.emit();
  }
}
