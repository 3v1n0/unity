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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include "SpacerLauncherIcon.h"
#include "Launcher.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

SpacerLauncherIcon::SpacerLauncherIcon(int monitor)
  : SingleMonitorLauncherIcon(monitor)
{
  SetQuirk(Quirk::VISIBLE, true);
  SetQuirk(Quirk::RUNNING, false);
  SetIconType(IconType::APPLICATION);

  tooltip_text = _("Drop To Add Application");
}

std::string SpacerLauncherIcon::GetName() const
{
    return "SpacerLauncherIcon";
}

} // namespace launcher
} // namespace unity
