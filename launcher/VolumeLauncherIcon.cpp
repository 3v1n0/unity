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
#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>
#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibSignal.h>

#include "VolumeLauncherIcon.h"
#include "FavoriteStore.h"

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon.volume");

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
    , file_manager_(fm)
  {
    UpdateIcon();
    UpdateVisibility();
    ConnectSignals();
  }

  void UpdateIcon()
  {
    parent_->tooltip_text = volume_->GetName();
    parent_->icon_name = volume_->GetIconName();
    parent_->SetQuirk(Quirk::RUNNING, file_manager_->IsPrefixOpened(volume_->GetUri()));
  }

  void UpdateVisibility()
  {
    UpdateKeepInLauncher();
    parent_->SetQuirk(Quirk::VISIBLE, keep_in_launcher_);
  }

  void UpdateKeepInLauncher()
  {
    auto const& identifier = volume_->GetIdentifier();
    keep_in_launcher_ = !devices_settings_->IsABlacklistedDevice(identifier);
  }

  void ConnectSignals()
  {
    connections_.Add(volume_->changed.connect(sigc::mem_fun(this, &Impl::OnVolumeChanged)));
    connections_.Add(volume_->removed.connect(sigc::mem_fun(this, &Impl::OnVolumeRemoved)));
    connections_.Add(devices_settings_->changed.connect(sigc::mem_fun(this, &Impl::OnSettingsChanged)));
    connections_.Add(file_manager_->locations_changed.connect(sigc::mem_fun(this, &Impl::UpdateIcon)));
  }

  void OnVolumeChanged()
  {
    UpdateIcon();
  }

  void OnVolumeRemoved()
  {
    if (devices_settings_->IsABlacklistedDevice(volume_->GetIdentifier()))
      devices_settings_->TryToUnblacklist(volume_->GetIdentifier());

    parent_->UnStick();
    parent_->Remove();
  }

  void OnSettingsChanged()
  {
    UpdateVisibility();
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
      notification_->Display(volume_->GetIconName(), volume_->GetName());
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
      file_manager_->OpenActiveChild(volume_->GetUri(), timestamp);
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
    AppendSeparatorItem(result);
    AppendNameItem(result);
    AppendSeparatorItem(result);
    AppendUnlockFromLauncherItem(result);
    AppendEjectItem(result);
    AppendSafelyRemoveItem(result);
    AppendUnmountItem(result);

    return result;
  }

  void AppendUnlockFromLauncherItem(MenuItemsVector& menu)
  {
    if (volume_->GetIdentifier().empty())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unlock from Launcher"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
      auto const& identifier = volume_->GetIdentifier();
      parent_->UnStick();
      devices_settings_->TryToBlacklist(identifier);
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
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, unsigned timestamp) {
      OpenInFileManager(timestamp);
    }));

    menu.push_back(menu_item);
  }

  void AppendOpenItem(MenuItemsVector& menu)
  {
    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, unsigned timestamp) {
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

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
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

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->StopDrive();
    }));

    menu.push_back(menu_item);
  }

  void AppendUnmountItem(MenuItemsVector& menu)
  {
    if (!volume_->IsMounted() || volume_->CanBeEjected() || volume_->CanBeStopped())
      return;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unmount"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->Unmount();
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
  bool keep_in_launcher_;
  Volume::Ptr volume_;
  DevicesSettings::Ptr devices_settings_;
  DeviceNotificationDisplay::Ptr notification_;
  FileManager::Ptr file_manager_;

  glib::SignalManager gsignals_;
  connection::Manager connections_;
};

//
// End private implementation
//

VolumeLauncherIcon::VolumeLauncherIcon(Volume::Ptr const& volume,
                                       DevicesSettings::Ptr const& devices_settings,
                                       DeviceNotificationDisplay::Ptr const& notification,
                                       FileManager::Ptr const& fm)
  : SimpleLauncherIcon(IconType::DEVICE)
  , pimpl_(new Impl(volume, devices_settings, notification, fm, this))
{}

VolumeLauncherIcon::~VolumeLauncherIcon()
{}

void VolumeLauncherIcon::AboutToRemove()
{
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

void VolumeLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  pimpl_->OpenInFileManager(arg.timestamp);
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

bool VolumeLauncherIcon::OnShouldHighlightOnDrag(DndData const& dnd_data)
{
  for (auto const& uri : dnd_data.Uris())
  {
    if (uri.find("file://") == 0)
      return true;
  }

  return false;
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
