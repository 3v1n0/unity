// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "Timer.h"

namespace unity
{
namespace util
{

Timer::Timer()
  : start_time_(g_get_monotonic_time())
{
}

void Timer::Reset()
{
  start_time_ = g_get_monotonic_time();
}

float Timer::ElapsedSeconds()
{
  return ElapsedMicroSeconds() / 1e6;
}

gint64 Timer::ElapsedMicroSeconds()
{
  gint64 end = g_get_monotonic_time();
  return end - start_time_;
}


}
}
