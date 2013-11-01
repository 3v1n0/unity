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

#include "DesktopLauncherIcon.h"
#include "unity-shared/WindowManager.h"
#include "FavoriteStore.h"
#include "config.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{

DesktopLauncherIcon::DesktopLauncherIcon()
  : SimpleLauncherIcon(IconType::DESKTOP)
  , show_in_switcher_(true)
{
  tooltip_text = _("Show Desktop");
  icon_name = "desktop";
  SetQuirk(Quirk::VISIBLE, true);
  SetShortcut('d');
}

void DesktopLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager::Default().ShowDesktop();
}

std::string DesktopLauncherIcon::GetName() const
{
  return "DesktopLauncherIcon";
}

std::string DesktopLauncherIcon::GetRemoteUri() const
{
  return FavoriteStore::URI_PREFIX_UNITY + "desktop-icon";
}

void DesktopLauncherIcon::SetShowInSwitcher(bool show_in_switcher)
{
  show_in_switcher_ = show_in_switcher;
}

bool DesktopLauncherIcon::ShowInSwitcher(bool current)
{
  return show_in_switcher_;
}

} // namespace launcher
} // namespace unity
