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
 *              Andrea Azzarone <andrea.azzarone@canonical.com>
 */


#include "config.h"
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSignal.h>

#include "VolumeLauncherIcon.h"
#include "FavoriteStore.h"

namespace unity
{
namespace launcher
{

//
// Start private implementation
//
class VolumeLauncherIcon::Impl
{
public:
  typedef glib::Signal<void, DbusmenuMenuitem*, unsigned> ItemSignal;

  Impl(Volume::Ptr const& volume,
       DevicesSettings::Ptr const& devices_settings,
       DeviceNotificationDisplay::Ptr const& notification,
       FileManager::Ptr const& fm,
       VolumeLauncherIcon* parent)
    : parent_(parent)
    , volume_(volume)
    , devices_settings_(devices_settings)
    , notification_(notification)
    , file_manager_(parent_->file_manager_)
  {
    UpdateIcon();
    UpdateVisibility();
    ConnectSignals();
  }

  void UpdateIcon()
  {
    parent_->tooltip_text = volume_->GetName();
    parent_->icon_name = volume_->GetIconName();
  }

  void UpdateVisibility()
  {
    parent_->SetQuirk(Quirk::VISIBLE, IsVisible());
  }

  bool IsBlackListed()
  {
    return devices_settings_->IsABlacklistedDevice(volume_->GetIdentifier());
  }

  bool IsVisible()
  {
    if (IsBlackListed() && parent_->GetManagedWindows().empty())
      return false;

    return true;
  }

  void ConnectSignals()
  {
    connections_.Add(volume_->changed.connect([this] { UpdateIcon(); }));
    connections_.Add(volume_->removed.connect(sigc::mem_fun(this, &Impl::OnVolumeRemoved)));
    connections_.Add(devices_settings_->changed.connect([this] { UpdateVisibility(); }));
    connections_.Add(parent_->windows_changed.connect([this] (int) { UpdateVisibility(); }));
  }

  void OnVolumeRemoved()
  {
    devices_settings_->TryToUnblacklist(volume_->GetIdentifier());
    parent_->UnStick();
    parent_->Remove();
  }

  bool CanEject() const
  {
    return volume_->CanBeEjected();
  }

  void EjectAndShowNotification()
  {
    if (!CanEject())
      return;

    auto conn = std::make_shared<sigc::connection>();
    *conn = volume_->ejected.connect([this, conn] {
      notification_->Display(volume_->GetName());
      conn->disconnect();
    });
    connections_.Add(*conn);
    volume_->Eject();
  }

  bool CanStop() const
  {
    return volume_->CanBeStopped();
  }

  void StopDrive()
  {
    volume_->StopDrive();
  }

  void DoActionWhenMounted(std::function<void()> const& callback)
  {
    if (!volume_->IsMounted())
    {
      auto conn = std::make_shared<sigc::connection>();
      *conn = volume_->mounted.connect([this, conn, callback] {
        callback();
        conn->disconnect();
      });
      connections_.Add(*conn);
      volume_->Mount();
    }
    else
    {
      callback();
    }
  }

  void OpenInFileManager(uint64_t timestamp)
  {
    DoActionWhenMounted([this, timestamp] {
      file_manager_->Open(volume_->GetUri(), timestamp);
    });
  }

  void CopyFilesToVolume(std::set<std::string> const& files, uint64_t timestamp)
  {
    DoActionWhenMounted([this, files, timestamp] {
      file_manager_->CopyFiles(files, volume_->GetUri(), timestamp);
    });
  }

  MenuItemsVector GetMenus()
  {
    MenuItemsVector result;

    AppendOpenItem(result);
    AppendFormatItem(result);
    AppendSeparatorItem(result);
    AppendNameItem(result);
    AppendSeparatorItem(result);
    AppendWindowsItems(result);
    AppendToggleLockFromLauncherItem(result);
    AppendEjectItem(result);
    AppendSafelyRemoveItem(result);
    AppendUnmountItem(result);
    AppendQuitItem(result);

    return result;
  }

  void AppendToggleLockFromLauncherItem(MenuItemsVector& menu)
  {
    if (volume_->GetIdentifier().empty())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    const char* label = IsBlackListed() ? _("Lock to Launcher") : _("Unlock from Launcher");
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, label);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      if (!IsBlackListed())
      {
        parent_->UnStick();
        devices_settings_->TryToBlacklist(volume_->GetIdentifier());
      }
      else
      {
        devices_settings_->TryToUnblacklist(volume_->GetIdentifier());
      }
    }));

    menu.push_back(menu_item);
  }

  void AppendSeparatorItem(MenuItemsVector& menu)
  {
    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
    menu.push_back(menu_item);
  }

  void AppendNameItem(MenuItemsVector& menu)
  {
    std::ostringstream bold_volume_name;
    bold_volume_name << "<b>" << volume_->GetName() << "</b>";

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, bold_volume_name.str().c_str());
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_ACCESSIBLE_DESC, volume_->GetName().c_str());
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, unsigned timestamp) {
      OpenInFileManager(timestamp);
    }));

    menu.push_back(menu_item);
  }

  void AppendWindowsItems(MenuItemsVector& menu)
  {
    if (!parent_->IsRunning())
      return;

    auto const& windows_items = parent_->GetWindowsMenuItems();
    if (!windows_items.empty())
    {
      menu.insert(end(menu), begin(windows_items), end(windows_items));
      AppendSeparatorItem(menu);
    }
  }

  void AppendOpenItem(MenuItemsVector& menu)
  {
    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, unsigned timestamp) {
      OpenInFileManager(timestamp);
    }));

    menu.push_back(menu_item);
  }

  void AppendEjectItem(MenuItemsVector& menu)
  {
    if (!volume_->CanBeEjected())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, volume_->HasSiblings() ? _("Eject parent drive") : _("Eject"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      parent_->Quit();
      EjectAndShowNotification();
    }));

    menu.push_back(menu_item);
  }

  void AppendSafelyRemoveItem(MenuItemsVector& menu)
  {
    if (!volume_->CanBeStopped())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, volume_->HasSiblings() ? _("Safely remove parent drive") : _("Safely remove"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      parent_->Quit();
      volume_->StopDrive();
    }));

    menu.push_back(menu_item);
  }

  void AppendFormatItem(MenuItemsVector& menu)
  {
    glib::Object<GDesktopAppInfo> gd(g_desktop_app_info_new("gnome-disks.desktop"));

    if (!volume_->CanBeFormatted() || !gd)
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Format…"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, unsigned timestamp) {
      OpenFormatPrompt(timestamp);
    }));

    menu.push_back(menu_item);
  }

  void OpenFormatPrompt(Time timestamp)
  {
    glib::Object<GDesktopAppInfo> gd_desktop_app_info(g_desktop_app_info_new("gnome-disks.desktop"));

    if (!gd_desktop_app_info)
      return;

    auto gd_app_info = glib::object_cast<GAppInfo>(gd_desktop_app_info);

    std::string command_line = glib::gchar_to_string(g_app_info_get_executable(gd_app_info)) +
                               " --block-device " +
                               volume_->GetUnixDevicePath() + 
                               " --format-device";

    GdkDisplay* display = gdk_display_get_default();
    glib::Object<GdkAppLaunchContext> app_launch_context(gdk_display_get_app_launch_context(display));
    gdk_app_launch_context_set_timestamp(app_launch_context, timestamp);

    glib::Object<GAppInfo> app_info(g_app_info_create_from_commandline(command_line.c_str(),
                                                                       nullptr,
                                                                       G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION,
                                                                       nullptr));
    g_app_info_launch_uris(app_info, nullptr, glib::object_cast<GAppLaunchContext>(app_launch_context), nullptr);
  }

  void AppendUnmountItem(MenuItemsVector& menu)
  {
    if (!volume_->IsMounted() || volume_->CanBeEjected() || volume_->CanBeStopped())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unmount"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      volume_->Unmount();
    }));

    menu.push_back(menu_item);
  }

  void AppendQuitItem(MenuItemsVector& menu)
  {
    if (!parent_->IsRunning())
      return;

    if (!menu.empty())
      AppendSeparatorItem(menu);

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Quit"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    parent_->glib_signals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      parent_->Quit();
    }));

    menu.push_back(menu_item);
  }

  std::string GetRemoteUri() const
  {
    auto const& identifier = volume_->GetIdentifier();

    if (identifier.empty())
      return "";

    return FavoriteStore::URI_PREFIX_DEVICE + identifier;
  }

  VolumeLauncherIcon* parent_;
  Volume::Ptr volume_;
  DevicesSettings::Ptr devices_settings_;
  DeviceNotificationDisplay::Ptr notification_;
  FileManager::Ptr file_manager_;
  connection::Manager connections_;
};

//
// End private implementation
//

VolumeLauncherIcon::VolumeLauncherIcon(Volume::Ptr const& volume,
                                       DevicesSettings::Ptr const& devices_settings,
                                       DeviceNotificationDisplay::Ptr const& notification,
                                       FileManager::Ptr const& fm)
  : WindowedLauncherIcon(IconType::DEVICE)
  , StorageLauncherIcon(GetIconType(), fm)
  , pimpl_(new Impl(volume, devices_settings, notification, fm, this))
{
  UpdateStorageWindows();
}

VolumeLauncherIcon::~VolumeLauncherIcon()
{}

void VolumeLauncherIcon::AboutToRemove()
{
  StorageLauncherIcon::AboutToRemove();

  if (CanEject())
    EjectAndShowNotification();
  else if (CanStop())
    StopDrive();
}

bool VolumeLauncherIcon::CanEject() const
{
  return pimpl_->CanEject();
}

void VolumeLauncherIcon::EjectAndShowNotification()
{
  pimpl_->EjectAndShowNotification();
}

bool VolumeLauncherIcon::CanStop() const
{
  return pimpl_->CanStop();
}

void VolumeLauncherIcon::StopDrive()
{
  return pimpl_->StopDrive();
}

AbstractLauncherIcon::MenuItemsVector VolumeLauncherIcon::GetMenus()
{
  return pimpl_->GetMenus();
}

std::string VolumeLauncherIcon::GetRemoteUri() const
{
  return pimpl_->GetRemoteUri();
}

void VolumeLauncherIcon::Stick(bool save)
{
  SimpleLauncherIcon::Stick(save);
  pimpl_->devices_settings_->TryToUnblacklist(pimpl_->volume_->GetIdentifier());
}

void VolumeLauncherIcon::UnStick()
{
  SimpleLauncherIcon::UnStick();
  SetQuirk(Quirk::VISIBLE, true);
}

nux::DndAction VolumeLauncherIcon::OnQueryAcceptDrop(DndData const& dnd_data)
{
  return dnd_data.Uris().empty() ? nux::DNDACTION_NONE : nux::DNDACTION_COPY;
}

void VolumeLauncherIcon::OnAcceptDrop(DndData const& dnd_data)
{
  auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  pimpl_->CopyFilesToVolume(dnd_data.Uris(), timestamp);
  SetQuirk(Quirk::PULSE_ONCE, true);
  FullyAnimateQuirkDelayed(100, LauncherIcon::Quirk::SHIMMER);
}

std::string VolumeLauncherIcon::GetVolumeUri() const
{
  return pimpl_->volume_->GetUri();
}

WindowList VolumeLauncherIcon::GetStorageWindows() const
{
  return file_manager_->WindowsForLocation(GetVolumeUri());
}

void VolumeLauncherIcon::OpenInstanceLauncherIcon(Time timestamp)
{
  pimpl_->OpenInFileManager(timestamp);
}

//
// Introspection
//
std::string VolumeLauncherIcon::GetName() const
{
  return "VolumeLauncherIcon";
}

} // namespace launcher
} // namespace unity
