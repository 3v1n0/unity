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

#include "config.h"

#include "PlaceLauncherIcon.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include <glib/gi18n-lib.h>

#define SECTION_NUMBER "ted-loves-strings"

PlaceLauncherIcon::PlaceLauncherIcon (Launcher *launcher, PlaceEntry *entry)
: SimpleLauncherIcon(launcher),
  _entry (entry)
{
  gchar *escape;

  escape = g_markup_escape_text (entry->GetName (), -1);

  SetTooltipText (escape);
  SetShortcut (entry->GetShortcut());
  SetIconName (entry->GetIcon ());
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, true);
  SetQuirk (QUIRK_ACTIVE, entry->IsActive ());
  SetIconType (TYPE_PLACE); 

  entry->active_changed.connect (sigc::mem_fun (this, &PlaceLauncherIcon::OnActiveChanged));

  g_free (escape);
}

PlaceLauncherIcon::~PlaceLauncherIcon()
{

}

nux::Color 
PlaceLauncherIcon::BackgroundColor ()
{
  return nux::Color (0xFF333333);
}

nux::Color 
PlaceLauncherIcon::GlowColor ()
{
  return nux::Color (0xFF333333);
}

void
PlaceLauncherIcon::OnMouseClick (int button)
{
  SimpleLauncherIcon::OnMouseClick (button);

  if (button == 1)
  {
    ActivateLauncherIcon ();
  }
}

void
PlaceLauncherIcon::ActivateLauncherIcon ()
{
  ActivatePlace (0, "");
}

void
PlaceLauncherIcon::UpdatePlaceIcon ()
{

}

void
PlaceLauncherIcon::ForeachSectionCallback (PlaceEntry *entry, PlaceEntrySection& section)
{
  DbusmenuMenuitem              *menu_item;
  char *temp;

  temp = g_markup_escape_text (section.GetName (), -1);
  menu_item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, section.GetName ());
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  dbusmenu_menuitem_property_set_int (menu_item, SECTION_NUMBER, _current_menu.size ());
  _current_menu.push_back (menu_item);
  g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (&PlaceLauncherIcon::OnOpen), this);
  g_free (temp);
}

std::list<DbusmenuMenuitem *>
PlaceLauncherIcon::GetMenus ()
{
  DbusmenuMenuitem              *menu_item;
  char * temp;
  
  _current_menu.erase (_current_menu.begin (), _current_menu.end ());

  _entry->ForeachSection (sigc::mem_fun (this, &PlaceLauncherIcon::ForeachSectionCallback));
  
  menu_item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  _current_menu.push_back (menu_item);

  temp = g_markup_escape_text (_entry->GetName (), -1);
  menu_item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, temp);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  dbusmenu_menuitem_property_set_int (menu_item, SECTION_NUMBER, 0);
  _current_menu.push_back (menu_item);
  g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (&PlaceLauncherIcon::OnOpen), this);
  g_free (temp);

  return _current_menu;
}

void
PlaceLauncherIcon::ActivatePlace (guint section_id, const char *search_string)
{
  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                            g_variant_new ("(sus)",
                                           _entry->GetId (),
                                           section_id,
                                           search_string));
}

void
PlaceLauncherIcon::OnOpen (DbusmenuMenuitem *item, int time, PlaceLauncherIcon *self)
{
  self->ActivatePlace (dbusmenu_menuitem_property_get_int (item, SECTION_NUMBER), "");
}

void
PlaceLauncherIcon::OnActiveChanged (bool is_active)
{
  SetQuirk (QUIRK_ACTIVE, is_active);
}
