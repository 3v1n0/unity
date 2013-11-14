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

#ifndef UNITYSHELL_SINGLE_MONITOR_LAUNCHER_ICON_H
#define UNITYSHELL_SINGLE_MONITOR_LAUNCHER_ICON_H

#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class SingleMonitorLauncherIcon : public SimpleLauncherIcon
{
public:
  SingleMonitorLauncherIcon(IconType type, int monitor = -1);

void SetMonitor(int monitor);
int GetMonitor() const;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void UpdateMonitor();

  int monitor_;
};

}
}

#endif // UNITYSHELL_SINGLE_MONITOR_LAUNCHER_ICON_H
