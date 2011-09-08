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
* Authored by: Alex Launi <alex.launi@canonical.com>
*/

#include "ElapsedTimeMonitor.h"
#include "TimeUtil.h"

namespace unity{
namespace performance {

gchar* ElapsedTimeMonitor::GetName()
{
  return (gchar*) "ElapsedTimeMonitor";
}

void ElapsedTimeMonitor::StartMonitor()
{
  clock_gettime(CLOCK_MONOTONIC, &_start);
}

void ElapsedTimeMonitor::StopMonitor(GVariantBuilder* builder)
{
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  int diff = TimeUtil::TimeDelta(&current, &_start);

  g_variant_builder_add(builder, "{sv}", "elapsed-time", g_variant_new_uint32(diff));
}

}
}
