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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "ExpoLauncherIcon.h"
#include "FavoriteStore.h"

#include "config.h"
#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

ExpoLauncherIcon::ExpoLauncherIcon()
  : SimpleLauncherIcon(IconType::EXPO)
{
  tooltip_text = _("Workspace Switcher");
  icon_name = "workspace-switcher-top-left";
  SetShortcut('s');

  auto& wm = WindowManager::Default();
  OnViewportLayoutChanged(wm.GetViewportHSize(), wm.GetViewportVSize());

  wm.viewport_layout_changed.connect(sigc::mem_fun(this, &ExpoLauncherIcon::OnViewportLayoutChanged));
}

void ExpoLauncherIcon::OnViewportLayoutChanged(int hsize, int vsize)
{
  if (hsize != 2 || vsize != 2)
  {
    icon_name = "workspace-switcher-top-left";
    viewport_changes_connections_.Clear();
  }
  else
  {
    UpdateIcon();

    if (viewport_changes_connections_.Empty())
    {
      auto& wm = WindowManager::Default();
      auto cb = sigc::mem_fun(this, &ExpoLauncherIcon::UpdateIcon);
      viewport_changes_connections_.Add(wm.screen_viewport_switch_ended.connect(cb));
      viewport_changes_connections_.Add(wm.terminate_expo.connect(cb));
    }
  }
}

void ExpoLauncherIcon::AboutToRemove()
{
  WindowManager::Default().SetViewportSize(1, 1);
}

void ExpoLauncherIcon::UpdateIcon()
{
  auto const& vp = WindowManager::Default().GetCurrentViewport();

  if (vp.x == 0 and vp.y == 0)
    icon_name = "workspace-switcher-top-left";
  else if (vp.x == 0)
    icon_name = "workspace-switcher-left-bottom";
  else if (vp.y == 0)
    icon_name = "workspace-switcher-right-top";
  else
    icon_name = "workspace-switcher-right-bottom";
}

void ExpoLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager& wm = WindowManager::Default();

  if (!wm.IsExpoActive())
    wm.InitiateExpo();
  else
    wm.TerminateExpo();
}

void ExpoLauncherIcon::Stick(bool save)
{
  SimpleLauncherIcon::Stick(save);
  SetQuirk(Quirk::VISIBLE, (WindowManager::Default().WorkspaceCount() > 1));
}

std::string ExpoLauncherIcon::GetName() const
{
  return "ExpoLauncherIcon";
}

std::string ExpoLauncherIcon::GetRemoteUri() const
{
  return FavoriteStore::URI_PREFIX_UNITY + "expo-icon";
}

} // namespace launcher
} // namespace unity
