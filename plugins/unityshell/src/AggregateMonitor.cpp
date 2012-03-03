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

#include "AggregateMonitor.h"
#include "ElapsedTimeMonitor.h"
#include <UnityCore/Variant.h>

namespace unity {
namespace performance {

AggregateMonitor::AggregateMonitor()
{
  _monitors.push_back(new ElapsedTimeMonitor());
}

AggregateMonitor::~AggregateMonitor()
{
}

std::string AggregateMonitor::GetName() const
{
  return "AggregateMonitor";
}

void AggregateMonitor::StartMonitor()
{
  for (std::list<Monitor*>::iterator iter = _monitors.begin(), end = _monitors.end();
       iter != end; ++iter)
  {
    Monitor* monitor = *iter;
    monitor->Start();
  }
}

void AggregateMonitor::StopMonitor(GVariantBuilder* builder)
{
  variant::BuilderWrapper wrapper(builder);
  for (std::list<Monitor*>::iterator iter = _monitors.begin(), end = _monitors.end();
       iter != end; ++iter)
  {
    Monitor* monitor = *iter;
    wrapper.add(monitor->GetName().c_str(), monitor->Stop());
  }
}

}
}
