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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Andrea Azzarone <azzaronea@gmail.com>
 */

#include "BFBLauncherIcon.h"
#include "Launcher.h"

#include "UBusMessages.h"

#include <glib/gi18n-lib.h>

namespace unity
{
namespace launcher
{
  
UBusManager BFBLauncherIcon::ubus_manager_;

BFBLauncherIcon::BFBLauncherIcon()
 : SimpleLauncherIcon()
{
  tooltip_text = _("Dash home");
  icon_name = PKGDATADIR"/launcher_bfb.png";
  SetQuirk(QUIRK_VISIBLE, true);
  SetQuirk(QUIRK_RUNNING, false);
  SetIconType(TYPE_HOME);
  
  background_color_ = nux::color::White;
  
  mouse_enter.connect([&](int m) { ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW, NULL); });
}

nux::Color BFBLauncherIcon::BackgroundColor()
{
  return background_color_;
}

nux::Color BFBLauncherIcon::GlowColor()
{
  return background_color_;
}

void BFBLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", "home.lens", 0, ""));

  // dont chain down to avoid random dash close events
}

void BFBLauncherIcon::OnMenuitemActivated(DbusmenuMenuitem* item,
                                          int time,
                                          gchar* lens)
{
  if (lens != NULL)
  {
    ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST, g_variant_new("(sus)", lens, dash::GOTO_DASH_URI, ""));
    g_free(lens);
  }
}

std::list<DbusmenuMenuitem*> BFBLauncherIcon::GetMenus()
{  
  std::list<DbusmenuMenuitem*> result;
  DbusmenuMenuitem* menu_item;
  
  // Home dash
  menu_item = dbusmenu_menuitem_new();
  
  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Dash Home"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  
  g_signal_connect(menu_item,
                   DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                   (GCallback)&BFBLauncherIcon::OnMenuitemActivated,
                   g_strdup("home.lens"));
  
  result.push_back(menu_item);
  
  // Other lenses..
  for (auto lens : lenses_.GetLenses())
  {
    if (!lens->visible())
      continue;
    
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, lens->name().c_str());
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    g_signal_connect(menu_item,
                     DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     (GCallback)&BFBLauncherIcon::OnMenuitemActivated,
                     g_strdup(lens->id().c_str()));
                     
    result.push_back(menu_item);
  }
  
  return result;
}

} // namespace launcher
} // namespace unity

