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
#include <algorithm>

namespace unity {

namespace {

#define DEFAULT_ICON "drive-removable-media"

} // anonymous namespace

DeviceLauncherIcon::DeviceLauncherIcon(Launcher *launcher, GVolume *volume)
  : SimpleLauncherIcon(launcher)
  , volume_(volume)
{

  DevicesSettings::GetDefault().changed.connect(sigc::mem_fun(this, &DeviceLauncherIcon::OnSettingsChanged));

  /* FIXME: implement it in DeviceLauncherSection */
  on_changed_handler_id_ = g_signal_connect(volume_,
                                            "changed",
                                            G_CALLBACK(&DeviceLauncherIcon::OnChanged),
                                            this);

  /* Checked if in favourites! */
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  DeviceList favorites = DevicesSettings::GetDefault().GetFavorites();
  DeviceList::iterator pos = std::find(favorites.begin(), favorites.end(), uuid.Str());

  checked_ = !(pos != favorites.end());
  
  UpdateDeviceIcon();
  UpdateVisibility();
}

void DeviceLauncherIcon::UpdateDeviceIcon()
{
  {
    glib::String name(g_volume_get_name(volume_));

    SetTooltipText(name.Value());
  }
  
  {
    glib::Object<GIcon> icon(g_volume_get_icon(volume_));
    glib::String icon_string(g_icon_to_string(icon.RawPtr()));

    SetIconName (icon_string.Value());
  }
  
  SetQuirk (QUIRK_RUNNING, false);
  SetIconType (TYPE_DEVICE);
}

inline nux::Color DeviceLauncherIcon::BackgroundColor ()
{
  return nux::Color (0xFF333333);
}

inline nux::Color DeviceLauncherIcon::GlowColor ()
{
  return nux::Color (0xFF333333);
}

std::list<DbusmenuMenuitem *> DeviceLauncherIcon::GetMenus()
{
  std::list<DbusmenuMenuitem *> result;
  DbusmenuMenuitem *menu_item;

  // "Keep in launcher" item
  if (DevicesSettings::GetDefault().GetDevicesOption() == DevicesSettings::ONLY_MOUNTED
      && !g_volume_should_automount(volume_))
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Keep in launcher"));
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_CHECK);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_int(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, checked_);
    
    g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(&DeviceLauncherIcon::OnTogglePin), this);

    result.push_back (menu_item);
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
  if (g_volume_can_eject(volume_))
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    g_signal_connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                     G_CALLBACK(&DeviceLauncherIcon::OnEject), this);

    result.push_back(menu_item);
  }

  // "Safely Remove" item (FIXME: Should it be "Safely remove"?)
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));

  if (drive.RawPtr() && g_drive_can_stop(drive.RawPtr()))
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Safely Remove"));
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

    if (mount.RawPtr() && g_mount_can_unmount(mount.RawPtr()))
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

void DeviceLauncherIcon::ShowMount(GMount *mount)
{
  glib::String name(g_volume_get_name(volume_));

  if (G_IS_MOUNT(mount))
  {
    glib::Object<GFile> root(g_mount_get_root(mount));

    if (G_IS_FILE(root.RawPtr()))
    {
      glib::String uri(g_file_get_uri(root.RawPtr()));
      glib::Error error;

      g_app_info_launch_default_for_uri(uri.Value(), NULL, error.AsOutParam());

      if (error)
        g_warning("Cannot open volume '%s': Unable to show %s: %s", name.Value(), uri.Value(), error.Message().c_str());
    }
    else
    {
      g_warning ("Cannot open volume '%s': Mount has no root", name.Value());
    }
  }
  else
  {
    g_warning ("Cannot open volume '%s': Mount-point is invalid", name.Value());
  }
}

void DeviceLauncherIcon::ActivateLauncherIcon()
{
  SetQuirk (QUIRK_STARTING, true);
  
  glib::Object<GMount> mount(g_volume_get_mount(volume_));

  if (G_IS_MOUNT(mount.RawPtr()))
    ShowMount (mount);
  else
    g_volume_mount(volume_,
                   (GMountMountFlags)0,
                   NULL,
                   NULL,
                   (GAsyncReadyCallback)&DeviceLauncherIcon::OnMountReady,
                   this);
}

void DeviceLauncherIcon::OnMountReady(GObject *object,
                                      GAsyncResult *result,
                                      DeviceLauncherIcon *self)
{
  glib::Error error;

  if (g_volume_mount_finish(self->volume_, result, error.AsOutParam()))
  {
    glib::Object<GMount> mount(g_volume_get_mount(self->volume_));
    self->ShowMount(mount.RawPtr());
  }
  else
  {
    glib::String name(g_volume_get_name(self->volume_));

    g_warning("Cannot open volume '%s': %s",
              name.Value(),
              error ? error.Message().c_str() : "Mount operation failed");
  }
}

void DeviceLauncherIcon::OnEjectReady(GObject *object,
                                      GAsyncResult *result,
                                      DeviceLauncherIcon *self)
{
  g_volume_eject_with_operation_finish(self->volume_, result, NULL);
}

void DeviceLauncherIcon::Eject()
{
  glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

  g_volume_eject_with_operation (volume_,
                                 (GMountUnmountFlags)0,
                                 mount_op.RawPtr(),
                                 NULL,
                                 (GAsyncReadyCallback)OnEjectReady,
                                 this);
}

void DeviceLauncherIcon::OnTogglePin(DbusmenuMenuitem *item,
                                     int time,
                                     DeviceLauncherIcon *self)
{
  glib::String uuid(g_volume_get_identifier(self->volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  
  self->checked_ = !self->checked_;

  if (self->checked_)
  {
    // If the volume is not mounted hide the icon
    glib::Object<GMount> mount (g_volume_get_mount(self->volume_));

    if (mount.RawPtr() == NULL)
      self->SetQuirk (QUIRK_VISIBLE, false); 

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

void DeviceLauncherIcon::OnOpen(DbusmenuMenuitem *item,
                                int time,
                                DeviceLauncherIcon *self)
{
  self->ActivateLauncherIcon();
}

void DeviceLauncherIcon::OnEject(DbusmenuMenuitem *item,
                                 int time,
                                 DeviceLauncherIcon *self)
{
  self->Eject();
}

void DeviceLauncherIcon::OnUnmountReady(GObject *object,
                                        GAsyncResult *result,
                                        DeviceLauncherIcon *self)
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

    g_mount_unmount_with_operation(mount.RawPtr(),
                                   (GMountUnmountFlags)0,
                                   op,
                                   NULL,
                                   (GAsyncReadyCallback)OnUnmountReady,
                                   this);
  }
}

void DeviceLauncherIcon::OnUnmount(DbusmenuMenuitem *item,
                                   int time,
                                   DeviceLauncherIcon *self)
{
  self->Unmount();
}

void DeviceLauncherIcon::OnRemoved ()
{
  if (on_changed_handler_id_ != 0)
    g_signal_handler_disconnect((gpointer) volume_, on_changed_handler_id_);

  Remove();
}

void DeviceLauncherIcon::OnDriveStop(DbusmenuMenuitem *item,
                                     int time,
                                     DeviceLauncherIcon *self)
{
  self->StopDrive();
}

void DeviceLauncherIcon::StopDrive()
{
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));
  glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

  g_drive_stop(drive.RawPtr(),
               (GMountUnmountFlags)0,
               mount_op.RawPtr(),
               NULL,
               (GAsyncReadyCallback)OnStopDriveReady,
               this);
}

void DeviceLauncherIcon::OnStopDriveReady(GObject *object,
                                          GAsyncResult *result,
                                          DeviceLauncherIcon *self)
{
  if (!self || !G_IS_VOLUME(self->volume_))
    return;

  glib::Object<GDrive> drive(g_volume_get_drive(self->volume_));
  g_drive_stop_finish (drive.RawPtr(), result, NULL);
}

void DeviceLauncherIcon::OnChanged(GVolume *volume, DeviceLauncherIcon *self)
{
  glib::Object<GMount> mount(g_volume_get_mount(self->volume_));

  if (DevicesSettings::GetDefault().GetDevicesOption() == DevicesSettings::ONLY_MOUNTED
      && mount.RawPtr() == NULL && self->checked_)
  {
    self->SetQuirk (QUIRK_VISIBLE, false); 
  } 
}

void DeviceLauncherIcon::UpdateVisibility()
{
  switch (DevicesSettings::GetDefault().GetDevicesOption())
  {
    case DevicesSettings::NEVER:
      SetQuirk (QUIRK_VISIBLE, false);
      break;
    case DevicesSettings::ONLY_MOUNTED:
    {
      glib::Object<GMount> mount(g_volume_get_mount(volume_));
      
      if (mount.RawPtr() == NULL && checked_)
        SetQuirk(QUIRK_VISIBLE, false); 
      else
        SetQuirk(QUIRK_VISIBLE, true);

      break;
    }
    case DevicesSettings::ALWAYS:
      SetQuirk(QUIRK_VISIBLE, true);
      break;
  }
}

void DeviceLauncherIcon::OnSettingsChanged()
{
  /* Checked if in favourites! */
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  DeviceList favorites = DevicesSettings::GetDefault().GetFavorites();
  DeviceList::iterator pos = std::find(favorites.begin(), favorites.end(), uuid.Str());

  checked_ = !(pos != favorites.end());

  UpdateVisibility();
}

} // namespace unity
