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
#include "unity-shared/IconLoader.h"
#include "unity-shared/ubus-server.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace launcher
{
namespace
{

nux::logging::Logger logger("unity.launcher");

const unsigned int volume_changed_timeout =  500;

}

DeviceLauncherIcon::DeviceLauncherIcon(glib::Object<GVolume> const& volume)
  : SimpleLauncherIcon(IconType::DEVICE)
  , volume_(volume)
{
  gsignals_.Add<void, GVolume*>(volume, "changed", sigc::mem_fun(this, &DeviceLauncherIcon::OnVolumeChanged));
  DevicesSettings::GetDefault().changed.connect(sigc::mem_fun(this, &DeviceLauncherIcon::OnSettingsChanged));

  // Checks if in favorites!
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));
  DeviceList favorites = DevicesSettings::GetDefault().GetFavorites();
  DeviceList::iterator pos = std::find(favorites.begin(), favorites.end(), uuid.Str());

  keep_in_launcher_ = pos != favorites.end();

  UpdateDeviceIcon();
  UpdateVisibility();
}

void DeviceLauncherIcon::OnVolumeChanged(GVolume* volume)
{
  if (!G_IS_VOLUME(volume))
    return;

  changed_timeout_.reset(new glib::Timeout(volume_changed_timeout, [this]() {
    UpdateDeviceIcon();
    UpdateVisibility();
    return false;
  }));
}

void DeviceLauncherIcon::UpdateVisibility()
{
  switch (DevicesSettings::GetDefault().GetDevicesOption())
  {
    case DevicesSettings::NEVER:
      SetQuirk(Quirk::VISIBLE, false);
      break;
    case DevicesSettings::ONLY_MOUNTED:
      if (keep_in_launcher_)
      {
        SetQuirk(Quirk::VISIBLE, true);
      }
      else
      {
        glib::Object<GMount> mount(g_volume_get_mount(volume_));
        SetQuirk(Quirk::VISIBLE, mount);
      }
      break;
    case DevicesSettings::ALWAYS:
      SetQuirk(Quirk::VISIBLE, true);
      break;
  }
}

void DeviceLauncherIcon::UpdateDeviceIcon()
{
  name_ = glib::String(g_volume_get_name(volume_)).Str();

  glib::Object<GIcon> icon(g_volume_get_icon(volume_));
  glib::String icon_string(g_icon_to_string(icon));

  tooltip_text = name_;
  icon_name = icon_string.Str();

  SetQuirk(Quirk::RUNNING, false);
}

bool
DeviceLauncherIcon::CanEject()
{
  return g_volume_can_eject(volume_);
}

AbstractLauncherIcon::MenuItemsVector DeviceLauncherIcon::GetMenus()
{
  MenuItemsVector result;
  glib::Object<DbusmenuMenuitem> menu_item;
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));
  typedef glib::Signal<void, DbusmenuMenuitem*, int> ItemSignal;

  // "Lock to Launcher"/"Unlock from Launcher" item
  if (DevicesSettings::GetDefault().GetDevicesOption() == DevicesSettings::ONLY_MOUNTED
      && drive && !g_drive_is_media_removable (drive))
  {
    menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, !keep_in_launcher_ ? _("Lock to Launcher") : _("Unlock from Launcher"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                 sigc::mem_fun(this, &DeviceLauncherIcon::OnTogglePin)));
    result.push_back(menu_item);
  }

  // "Open" item
  menu_item = dbusmenu_menuitem_new();

  dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
  dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

  gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
    ActivateLauncherIcon(ActionArg(ActionArg::OTHER, 0));
  }));

  result.push_back(menu_item);

  // "Eject" item
  if (drive && g_drive_can_eject(drive))
  {
    menu_item = dbusmenu_menuitem_new();

    GList *list = g_drive_get_volumes(drive);
    if (list)
    {
      if (!list->next) // If the list has only one item
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject"));
      else
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Eject parent drive"));

      g_list_free_full(list, g_object_unref);
    }

    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      Eject();
    }));

    result.push_back(menu_item);
  }

  // "Safely remove" item
  if (drive && g_drive_can_stop(drive))
  {
    menu_item = dbusmenu_menuitem_new();

    GList *list = g_drive_get_volumes(drive);
    if (list)
    {
      if (!list->next) // If the list has only one item
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Safely remove"));
      else
        dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Safely remove parent drive"));

      g_list_free_full(list, g_object_unref);
    }

    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      StopDrive();
    }));

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

      gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        Unmount();
      }));

      result.push_back(menu_item);
    }
  }

  return result;
}

void DeviceLauncherIcon::ShowMount(GMount* mount)
{
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
        LOG_WARNING(logger) << "Cannot open volume '" << name_
                            << "': Unable to show " << uri
                            << ": " << error;
      }
    }
    else
    {
      LOG_WARNING(logger) << "Cannot open volume '" << name_
                          << "': Mount has no root";
    }
  }
  else
  {
    LOG_WARNING(logger) << "Cannot open volume '" << name_
                        << "': Mount-point is invalid";
  }
}

void DeviceLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  SetQuirk(Quirk::STARTING, true);

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
    LOG_WARNING(logger) << "Cannot open volume '" << self->name_ << "' : " <<
                            (error ? error.Message() : "Mount operation failed");
  }
}

void DeviceLauncherIcon::OnEjectReady(GObject* object,
                                      GAsyncResult* result,
                                      DeviceLauncherIcon* self)
{
  if (g_volume_eject_with_operation_finish(self->volume_, result, NULL))
  {
    IconLoader::GetDefault().LoadFromGIconString(self->icon_name(), 48,
                                                 sigc::bind(sigc::mem_fun(self, &DeviceLauncherIcon::ShowNotification), self->name_));
  }
}

void DeviceLauncherIcon::ShowNotification(std::string const& icon_name,
                                          unsigned size,
                                          glib::Object<GdkPixbuf> const& pixbuf,
                                          std::string const& name)
{
  glib::Object<NotifyNotification> notification(notify_notification_new(name.c_str(),
                                                                        _("The drive has been successfully ejected"),
                                                                        NULL));

  notify_notification_set_hint(notification,
                               "x-canonical-private-synchronous",
                               g_variant_new_boolean(TRUE));

  if (GDK_IS_PIXBUF(pixbuf.RawPtr()))
    notify_notification_set_image_from_pixbuf(notification, pixbuf);

  notify_notification_show(notification, NULL);
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

void DeviceLauncherIcon::OnTogglePin(DbusmenuMenuitem* item, int time)
{
  glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));

  keep_in_launcher_ = !keep_in_launcher_;

  if (!keep_in_launcher_)
  {
    // If the volume is not mounted hide the icon
    glib::Object<GMount> mount(g_volume_get_mount(volume_));

    if (!mount)
      SetQuirk(Quirk::VISIBLE, false);

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

    g_mount_unmount_with_operation(mount, (GMountUnmountFlags)0, op, NULL,
                                   (GAsyncReadyCallback)OnUnmountReady, this);
  }
}

void DeviceLauncherIcon::OnRemoved()
{
  Remove();
}

void DeviceLauncherIcon::StopDrive()
{
  glib::Object<GDrive> drive(g_volume_get_drive(volume_));
  glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

  g_drive_stop(drive, (GMountUnmountFlags)0, mount_op, NULL, NULL, NULL);
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

std::string DeviceLauncherIcon::GetName() const
{
  return "DeviceLauncherIcon";
}

} // namespace launcher
} // namespace unity
