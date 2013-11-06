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

#ifndef UNITY_PERFORMANCE_ELAPSED_TIME_MONITOR
#define UNITY_PERFORMANCE_ELAPSED_TIME_MONITOR

#include <glib.h>
#include <time.h>

#include "Monitor.h"

namespace unity {
namespace performance {

class ElapsedTimeMonitor : public Monitor
{
public:
  virtual ~ElapsedTimeMonitor() {}
  std::string GetName() const;

protected:
  void StartMonitor();
  void StopMonitor(GVariantBuilder* builder);

private:
  struct timespec _start;
};

}
}

#endif // UNITY_PERFORMANCE_ELAPSED_TIME_MONITOR
