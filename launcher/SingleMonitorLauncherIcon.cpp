// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include "SingleMonitorLauncherIcon.h"

namespace unity
{
namespace launcher
{

SingleMonitorLauncherIcon::SingleMonitorLauncherIcon(IconType type, int monitor)
 : SimpleLauncherIcon(type)
 , monitor_(monitor)
{
  UpdateMonitor();
}

void SingleMonitorLauncherIcon::UpdateMonitor()
{
  for (unsigned i = 0; i < monitors::MAX; ++i)
    SetVisibleOnMonitor(i, static_cast<int>(i) == monitor_);
}

void SingleMonitorLauncherIcon::SetMonitor(int monitor)
{
  if (monitor != monitor_)
  {
    monitor_ = monitor;
    UpdateMonitor();
  }
}

int SingleMonitorLauncherIcon::GetMonitor() const
{
  return monitor_;
}

std::string SingleMonitorLauncherIcon::GetName() const
{
  return "SingleMonitorLauncherIcon";
}

void SingleMonitorLauncherIcon::AddProperties(debug::IntrospectionData& introspection)
{
  SimpleLauncherIcon::AddProperties(introspection);
  introspection.add("monitor", monitor_);
}

} // namespace launcher
} // namespace unity

