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

AbstractLauncherIcon::MenuItemsVector ExpoLauncherIcon::GetMenus()
{
  MenuItemsVector result;
  glib::Object<DbusmenuMenuitem> menu_item;
  typedef glib::Signal<void, DbusmenuMenuitem*, int> ItemSignal;

  auto& wm = WindowManager::Default();
  int h_size = wm.GetViewportHSize();
  int v_size = wm.GetViewportVSize();
  auto const& current_vp = wm.GetCurrentViewport();

  for (int h = 0; h < h_size; ++h)
  {
    for (int v = 0; v < v_size; ++v)
    {
      menu_item = dbusmenu_menuitem_new();
      glib::String label((v_size < 2) ? g_strdup_printf(_("Workspace %d"), (h+1)) :
                                        g_strdup_printf(_("Workspace %dx%d"), (h+1), (v+1)));
      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, label);
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

      if (current_vp.x == h && current_vp.y == v)
      {
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
        dbusmenu_menuitem_property_set_int(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
      }

      signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this, h, v] (DbusmenuMenuitem*, int) {
        WindowManager::Default().SetCurrentViewport({h, v});
        UpdateIcon();
      }));
      result.push_back(menu_item);
    }
  }

  return result;
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
