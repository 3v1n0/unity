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

#include "ExpoLauncherIcon.h"
#include "unity-shared/WindowManager.h"
#include "FavoriteStore.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

ExpoLauncherIcon::ExpoLauncherIcon()
  : SimpleLauncherIcon(IconType::EXPO)
{
  tooltip_text = _("Workspace Switcher");
  icon_name = "workspace-switcher";
  SetQuirk(Quirk::VISIBLE, false);
  SetQuirk(Quirk::RUNNING, false);
  SetShortcut('s');
}

void ExpoLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);

  auto wm = WindowManager::Default();

  if (!wm->IsExpoActive())
  {
    wm->InitiateExpo();
  }
  else
  {
    wm->TerminateExpo();
  }
}

void ExpoLauncherIcon::Stick(bool save)
{
  SimpleLauncherIcon::Stick(save);
  SetQuirk(Quirk::VISIBLE, (WindowManager::Default()->WorkspaceCount() > 1));
}

std::string ExpoLauncherIcon::GetName() const
{
  return "ExpoLauncherIcon";
}

std::string ExpoLauncherIcon::GetRemoteUri()
{
  return FavoriteStore::URI_PREFIX_UNITY + "expo-icon";
}

} // namespace launcher
} // namespace unity
