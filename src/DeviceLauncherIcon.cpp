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

#include <glib/gi18n-lib.h>

#define DEFAULT_ICON "drive-removable-media"

DeviceLauncherIcon::DeviceLauncherIcon (Launcher *launcher, GVolume *volume)
: SimpleLauncherIcon(launcher),
  _volume (volume)
{
  g_signal_connect (_volume, "removed",
                    G_CALLBACK (&DeviceLauncherIcon::OnRemoved), this);

  UpdateDeviceIcon ();

}

DeviceLauncherIcon::~DeviceLauncherIcon()
{

}

void
DeviceLauncherIcon::UpdateDeviceIcon ()
{
  {
    gchar *name;
    gchar *escape;

    name = g_volume_get_name (_volume);
    escape = g_markup_escape_text (name, -1);

    SetTooltipText (escape);

    g_free (escape);
    g_free (name);
  }
  
  {
    GIcon *icon;
    gchar *icon_string;

    icon = g_volume_get_icon (_volume);
    icon_string = g_icon_to_string (icon);

    SetIconName (icon_string);

    g_object_unref (icon);
    g_free (icon_string);
  }
  
  SetQuirk (QUIRK_VISIBLE, true);
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_DEVICE);
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
    ActivateLauncherIcon ();
  }
}

std::list<DbusmenuMenuitem *>
DeviceLauncherIcon::GetMenus ()
{
  std::list<DbusmenuMenuitem *>  result;
  DbusmenuMenuitem              *menu_item;

  menu_item = dbusmenu_menuitem_new ();
  dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
  g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                    G_CALLBACK (&DeviceLauncherIcon::OnOpen), this);
  result.push_back (menu_item);

  if (g_volume_can_eject (_volume))
  {
    menu_item = dbusmenu_menuitem_new ();
    dbusmenu_menuitem_property_set (menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject"));
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool (menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    g_signal_connect (menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                      G_CALLBACK (&DeviceLauncherIcon::OnEject), this);
    result.push_back (menu_item);
  }

  return result;
}

void
DeviceLauncherIcon::ShowMount (GMount *mount)
{
  gchar  *name;

  name = g_volume_get_name (_volume);
  if (G_IS_MOUNT (mount))
  {
    GFile *root;

    root = g_mount_get_root (mount);
    if (G_IS_FILE (root))
    {
      gchar  *uri;
      GError *error = NULL;

      uri = g_file_get_uri (root);

      g_app_info_launch_default_for_uri (uri, NULL, &error);
      if (error)
      {
        g_warning ("Cannot open volume '%s': Unable to show %s: %s", name, uri, error->message);
        g_error_free (error);
      }

      g_free (uri);
      g_object_unref (root);
    }
    else
    {
      g_warning ("Cannot open volume '%s': Mount has no root", name);
    }
  }
  else
  {
    g_warning ("Cannot open volume '%s': Mount-point is invalid", name);
  }

  g_free (name);
}

void
DeviceLauncherIcon::ActivateLauncherIcon ()
{
  GMount *mount;
  gchar  *name;

  SetQuirk (QUIRK_STARTING, true);
  
  name = g_volume_get_name (_volume);
  mount = g_volume_get_mount (_volume);
  if (G_IS_MOUNT (mount))
  {
    ShowMount (mount);
    g_object_unref (mount);
  }
  else
  {
    g_volume_mount (_volume,
                    (GMountMountFlags)0,
                    NULL,
                    NULL,
                    (GAsyncReadyCallback)&DeviceLauncherIcon::OnMountReady,
                    this);
  }

  g_free (name);
}

void
DeviceLauncherIcon::OnMountReady (GObject *object, GAsyncResult *result, DeviceLauncherIcon *self)
{
  GError *error = NULL;

  if (g_volume_mount_finish (self->_volume, result, &error))
  {
    GMount *mount;

    mount = g_volume_get_mount (self->_volume);
    self->ShowMount (mount);

    g_object_unref (mount);
  }
  else
  {
    gchar *name;

    name = g_volume_get_name (self->_volume);
    g_warning ("Cannot open volume '%s': %s",
               name,
               error ? error->message : "Mount operation failed");
    g_error_free (error);
  }
}

void
DeviceLauncherIcon::OnEjectReady (GObject            *object,
                                  GAsyncResult       *result,
                                  DeviceLauncherIcon *self)
{
  g_volume_eject_with_operation_finish (self->_volume, result, NULL);
}

void
DeviceLauncherIcon::Eject ()
{
  g_debug ("%s", G_STRLOC);
  g_volume_eject_with_operation (_volume,
                                 (GMountUnmountFlags)0,
                                 NULL,
                                 NULL,
                                 (GAsyncReadyCallback)OnEjectReady,
                                 this);
  g_debug ("%s", G_STRLOC);
}

void
DeviceLauncherIcon::OnOpen (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self)
{
  self->ActivateLauncherIcon ();
}

void
DeviceLauncherIcon::OnEject (DbusmenuMenuitem *item, int time, DeviceLauncherIcon *self)
{
  g_debug ("%s", G_STRLOC);
  self->Eject ();
  g_debug ("%s", G_STRLOC);
}

void
DeviceLauncherIcon::OnRemoved (GVolume *volume, DeviceLauncherIcon *self)
{
  self->Remove ();
}
