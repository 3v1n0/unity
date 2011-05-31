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

#include <ostream>

namespace unity {
namespace logger {

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
  gint64 end = g_get_monotonic_time();
  return (end - start_time_) / 1e6;
}


BlockTimer::BlockTimer(std::string const& name, std::ostream& out)
  : name_(name)
  , out_(out)
{
  out_ << "         STARTED (" << name_ << ")" << "\n";
}

BlockTimer::~BlockTimer()
{
  out_ << timer_.ElapsedSeconds() << "s: FINISHED ("
       << name_ << ")" << "\n";
}

void BlockTimer::log(std::string const& message)
{
  out_ << timer_.ElapsedSeconds() << "s: " << message
       << " (" << name_ << ")" << "\n";
}

}
}
