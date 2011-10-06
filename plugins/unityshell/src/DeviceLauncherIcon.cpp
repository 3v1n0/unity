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

#include <algorithm>
#include <list>

#include <glib/gi18n-lib.h>
#include <libnotify/notify.h>
#include <NuxCore/Logger.h>

#include "DevicesSettings.h"
#include "ubus-server.h"
#include "UBusMessages.h"

namespace unity
{
namespace launcher
{
namespace
{
nux::logging::Logger logger("unity.launcher");
}

DeviceLauncherIcon::DeviceLauncherIcon(Launcher* launcher, GVolume* volume)
  : SimpleLauncherIcon(launcher)
  , volume_(volume)
{
  DevicesSettings::GetDefault().changed.connect(sigc::mem_fun(this, &DeviceLauncherIcon::OnSettingsChanged));

  // Checks if in favourites!
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  DeviceList favorites = DevicesSettings::GetDefault().GetFavorites();
  DeviceList::iterator pos = std::find(favorites.begin(), favorites.end(), uuid.Str());

  keep_in_launcher_ = pos != favorites.end();
  
  UpdateDeviceIcon();
  UpdateVisibility();
}

void DeviceLauncherIcon::UpdateDeviceIcon()
{
  glib::String name(g_volume_get_name(volume_));
  glib::Object<GIcon> icon(g_volume_get_icon(volume_));
  glib::String icon_string(g_icon_to_string(icon));

  tooltip_text = name.Value();
  SetIconName(icon_string.Value());
  
  SetIconType(TYPE_DEVICE);
  SetQuirk(QUIRK_RUNNING, false);
}

bool
DeviceLauncherIcon::CanEject()
{
  return g_volume_can_eject(volume_);
}

nux::Color DeviceLauncherIcon::BackgroundColor()
{
  return nux::Color(0xFF333333);
}

nux::Color DeviceLauncherIcon::GlowColor()
{
  return nux::Color(0xFF333333);
}

std::list<DbusmenuMenuitem*> DeviceLauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem*> result;
  DbusmenuMenuitem* menu_item;
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));

  // "Keep in launcher" item
  if (DevicesSettings::GetDefault().GetDevicesOption() == DevicesSettings::ONLY_MOUNTED
      && drive && !g_drive_is_media_removable (drive))
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Keep in launcher"));
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_int(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, !keep_in_launcher_);
    
    g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(&DeviceLauncherIcon::OnTogglePin), this);

    result.push_back(menu_item);
  }
  
  // "Open" item
  menu_item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                   G_CALLBACK(&DeviceLauncherIcon::OnOpen), this);
                   
  result.push_back(menu_item);

  // "Eject" item
  if (drive && g_drive_can_eject(drive))
  {    
    menu_item = dbusmenu_menuitem_new();
    
    GList *list = g_drive_get_volumes(drive);
    if (list != NULL)
    {
      if (g_list_length (list) ==  1)
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject"));
      else
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject parent drive"));
        
      g_list_free_full(list, g_object_unref);
    }
    
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(&DeviceLauncherIcon::OnEject), this);

    result.push_back(menu_item);
  }

  // "Safely remove" item
  if (drive && g_drive_can_stop(drive))
  {
    menu_item = dbusmenu_menuitem_new();
    
    GList *list = g_drive_get_volumes(drive);
    if (list != NULL)
    {
      if (g_list_length (list) ==  1)
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Safely remove"));
      else
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Safely remove parent drive"));
        
      g_list_free_full(list, g_object_unref);
    }

    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(&DeviceLauncherIcon::OnDriveStop), this);
                     
    result.push_back(menu_item);
  }

  // "Unmount" item
  if (!g_volume_can_eject(volume_))  // Don't need Unmount if can Eject
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));

    if (mount && g_mount_can_unmount(mount))
    {
      menu_item = dbusmenu_menuitem_new();

      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unmount"));
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
      dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

      g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                       G_CALLBACK(&DeviceLauncherIcon::OnUnmount), this);

      result.push_back(menu_item);
    }
  }

  return result;
}

void DeviceLauncherIcon::ShowMount(GMount* mount)
{
  glib::String name(g_volume_get_name(volume_));

  if (G_IS_MOUNT(mount))
  {
    glib::Object<GFile> root(g_mount_get_root(mount));

    if (G_IS_FILE(root.RawPtr()))
    {
      glib::String uri(g_file_get_uri(root));
      glib::Error error;

      g_app_info_launch_default_for_uri(uri.Value(), NULL, &error);

      if (error)
      {
        LOG_WARNING(logger) << "Cannot open volume '" << name
                            << "': Unable to show " << uri
                            << ": " << error;
      }
    }
    else
    {
      LOG_WARNING(logger) << "Cannot open volume '" << name
                          << "': Mount has no root";
    }
  }
  else
  {
    LOG_WARNING(logger) << "Cannot open volume '" << name
                        << "': Mount-point is invalid";
  }
}

void DeviceLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  SetQuirk(QUIRK_STARTING, true);
  
  glib::Object<GMount> mount(g_volume_get_mount(volume_));

  if (G_IS_MOUNT(mount.RawPtr()))
    ShowMount(mount);
  else
    g_volume_mount(volume_,
                   (GMountMountFlags)0,
                   NULL,
                   NULL,
                   (GAsyncReadyCallback)&DeviceLauncherIcon::OnMountReady,
                   this);
}

void DeviceLauncherIcon::OnMountReady(GObject* object,
                                      GAsyncResult* result,
                                      DeviceLauncherIcon* self)
{
  glib::Error error;

  if (g_volume_mount_finish(self->volume_, result, &error))
  {
    glib::Object<GMount> mount(g_volume_get_mount(self->volume_));
    self->ShowMount(mount);
  }
  else
  {
    glib::String name(g_volume_get_name(self->volume_));

    g_warning("Cannot open volume '%s': %s",
              name.Value(),
              error ? error.Message().c_str() : "Mount operation failed");
  }
}

void DeviceLauncherIcon::OnEjectReady(GObject* object,
                                      GAsyncResult* result,
                                      DeviceLauncherIcon* self)
{
  if (g_volume_eject_with_operation_finish(self->volume_, result, NULL))
  {
    NotifyNotification* notification;
    glib::String name(g_volume_get_name(self->volume_));

    notification = notify_notification_new(name.Value(),
                                           _("The drive has been successfully ejected"),
                                           "drive-removable-media-usb");

    notify_notification_show(notification, NULL);
  }
}

void DeviceLauncherIcon::Eject()
{
  glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

  g_volume_eject_with_operation(volume_,
                                (GMountUnmountFlags)0,
                                mount_op,
                                NULL,
                                (GAsyncReadyCallback)OnEjectReady,
                                this);
}

void DeviceLauncherIcon::OnTogglePin(DbusmenuMenuitem* item,
                                     int time,
                                     DeviceLauncherIcon* self)
{
  glib::String uuid(g_volume_get_identifier(self->volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  
  self->keep_in_launcher_ = !self->keep_in_launcher_;

  if (!self->keep_in_launcher_)
  {
    // If the volume is not mounted hide the icon
    glib::Object<GMount> mount(g_volume_get_mount(self->volume_));

    if (!mount)
      self->SetQuirk(QUIRK_VISIBLE, false); 

    // Remove from favorites
    if (!uuid.Str().empty())
      DevicesSettings::GetDefault().RemoveFavorite(uuid.Str());
  }
  else
  {
    if (!uuid.Str().empty())
      DevicesSettings::GetDefault().AddFavorite(uuid.Str());
  }
}

void DeviceLauncherIcon::OnOpen(DbusmenuMenuitem* item,
                                int time,
                                DeviceLauncherIcon* self)
{
  self->ActivateLauncherIcon(ActionArg(ActionArg::OTHER, 0));
}

void DeviceLauncherIcon::OnEject(DbusmenuMenuitem* item,
                                 int time,
                                 DeviceLauncherIcon* self)
{
  self->Eject();
}

void DeviceLauncherIcon::OnUnmountReady(GObject* object,
                                        GAsyncResult* result,
                                        DeviceLauncherIcon* self)
{
  if (G_IS_MOUNT(object))
    g_mount_unmount_with_operation_finish(G_MOUNT(object), result, NULL);
}

void DeviceLauncherIcon::Unmount()
{
  glib::Object<GMount> mount(g_volume_get_mount(volume_));
  
  if (mount)
  {
    glib::Object<GMountOperation> op(gtk_mount_operation_new(NULL));

    g_mount_unmount_with_operation(mount,
                                   (GMountUnmountFlags)0,
                                   op,
                                   NULL,
                                   (GAsyncReadyCallback)OnUnmountReady,
                                   this);
  }
}

void DeviceLauncherIcon::OnUnmount(DbusmenuMenuitem* item,
                                   int time,
                                   DeviceLauncherIcon* self)
{
  self->Unmount();
}

void DeviceLauncherIcon::OnRemoved()
{
  Remove();
}

void DeviceLauncherIcon::OnDriveStop(DbusmenuMenuitem* item,
                                     int time,
                                     DeviceLauncherIcon* self)
{
  self->StopDrive();
}

void DeviceLauncherIcon::StopDrive()
{
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));
  glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

  g_drive_stop(drive,
               (GMountUnmountFlags)0,
               mount_op,
               NULL,
               NULL,
               NULL);
}

void DeviceLauncherIcon::UpdateVisibility(int visibility)
{
  switch (DevicesSettings::GetDefault().GetDevicesOption())
  {
    case DevicesSettings::NEVER:
      SetQuirk(QUIRK_VISIBLE, false);
      break;
    case DevicesSettings::ONLY_MOUNTED:
      if (keep_in_launcher_)
      {
        SetQuirk(QUIRK_VISIBLE, true);
      }
      else if (visibility < 0)
      {
        glib::Object<GMount> mount(g_volume_get_mount(volume_));
        SetQuirk(QUIRK_VISIBLE, mount.RawPtr() != NULL); 
      }
      else
      {
        SetQuirk(QUIRK_VISIBLE, visibility); 
      }
      break;
    case DevicesSettings::ALWAYS:
      SetQuirk(QUIRK_VISIBLE, true);
      break;
  }
}

void DeviceLauncherIcon::OnSettingsChanged()
{
  // Checks if in favourites!
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  DeviceList favorites = DevicesSettings::GetDefault().GetFavorites();
  DeviceList::iterator pos = std::find(favorites.begin(), favorites.end(), uuid.Str());

  keep_in_launcher_ = pos != favorites.end();

  UpdateVisibility();
}

} // namespace launcher
} // namespace unity
