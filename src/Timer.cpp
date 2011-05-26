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

Timer::Timer(std::string const& name, std::ostream& out)
  : name_(name)
  , out_(out)
  , start_time_(g_get_monotonic_time())
{
  out_ << "STARTED (" << name_ << ")" << "\n";
}

Timer::~Timer()
{
  gint64 end = g_get_monotonic_time();
  out_ << ((end - start_time_) / 1000.0) << ": FINISHED ("
       << name_ << ")" << "\n";
}

void Timer::log(std::string const& message)
{
  gint64 now = g_get_monotonic_time();
  out_ << ((now - start_time_) / 1000.0) << ": " << message
       << " (" << name_ << ")" << "\n";
}

}
}
