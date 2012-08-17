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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "SpreadLauncherIcon.h"
#include "unity-shared/WindowManager.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

SpreadLauncherIcon::SpreadLauncherIcon()
  : SimpleLauncherIcon(IconType::EXPO)
{
  tooltip_text = _("Workspace Switcher");
  icon_name = "workspace-switcher";
  SetQuirk(Quirk::VISIBLE, true);
  SetQuirk(Quirk::RUNNING, false);
  SetShortcut('s');
}

void SpreadLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager::Default()->InitiateExpo();
}

std::string SpreadLauncherIcon::GetName() const
{
  return "SpreadLauncherIcon";
}

std::string SpreadLauncherIcon::GetRemoteUri()
{
  return "unity://spread";
}

} // namespace launcher
} // namespace unity
