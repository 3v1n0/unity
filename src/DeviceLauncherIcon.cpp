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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "DeviceLauncherIcon.h"

#include "ubus-server.h"
#include "UBusMessages.h"

DeviceLauncherIcon::DeviceLauncherIcon (Launcher *launcher, GVolume *volume)
: SimpleLauncherIcon(launcher),
  _volume (volume)
{
  gchar *name;
  gchar *escape;

  name = g_volume_get_name (volume);
  escape = g_markup_escape_text (name, -1);

  SetTooltipText (escape);
  SetIconName ("volume");
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_DEVICE);

  g_signal_connect (_volume, "removed",
                    G_CALLBACK (&DeviceLauncherIcon::OnRemoved), this);

  g_free (escape);
  g_free (name);
}

DeviceLauncherIcon::~DeviceLauncherIcon()
{

}

nux::Color 
DeviceLauncherIcon::BackgroundColor ()
{
  return nux::Color (0xFF333333);
}

nux::Color 
DeviceLauncherIcon::GlowColor ()
{
  return nux::Color (0xFF333333);
}

void
DeviceLauncherIcon::OnMouseClick (int button)
{
  SimpleLauncherIcon::OnMouseClick (button);

  if (button == 1)
  {
    Activate ();
  }
}

void
DeviceLauncherIcon::UpdateDeviceIcon ()
{

}

std::list<DbusmenuMenuitem *>
DeviceLauncherIcon::GetMenus ()
{
  std::list<DbusmenuMenuitem *>  result;
  DbusmenuMenuitem              *menu_item;

  menu_item = dbusmenu_menuitem_new ();

  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, "Open");
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (&DeviceLauncherIcon::OnOpen), this);

  result.push_back (menu_item);

  return result;
}

void
DeviceLauncherIcon::Activate ()
{
}

void
DeviceLauncherIcon::OnOpen (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self)
{
  self->Activate ();
}

void
DeviceLauncherIcon::OnRemoved (GVolume *volume, DeviceLauncherIcon *self)
{
  self->Remove ();
}
