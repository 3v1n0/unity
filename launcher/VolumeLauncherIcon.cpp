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

#include "DevicesSettings.h"
#include "Volume.h"
#include "VolumeLauncherIcon.h"
#include "FavoriteStore.h"

namespace unity
{
namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher.icon");
namespace
{
const unsigned int volume_changed_timeout = 500;

}

//
// Start private implementation
//
class VolumeLauncherIcon::Impl
{
public:
  typedef glib::Signal<void, DbusmenuMenuitem*, unsigned> ItemSignal;

  Impl(Volume::Ptr const& volume,
       DevicesSettings::Ptr const& devices_settings,
       VolumeLauncherIcon* parent)
    : parent_(parent)
    , volume_(volume)
    , devices_settings_(devices_settings)
  {
    UpdateIcon();
    UpdateVisibility();
    ConnectSignals();
  }

  void UpdateIcon()
  {
    parent_->tooltip_text = volume_->GetName();
    parent_->icon_name = volume_->GetIconName();
    parent_->SetQuirk(Quirk::RUNNING, volume_->IsOpened());
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
    connections_.Add(volume_->opened.connect(sigc::hide(sigc::mem_fun(this, &Impl::UpdateIcon))));
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
    return volume_->EjectAndShowNotification();
  }

  bool CanStop() const
  {
    return volume_->CanBeStopped();
  }

  void StopDrive()
  {
    volume_->StopDrive();
  }

  void ActivateLauncherIcon(ActionArg arg)
  {
    parent_->SimpleLauncherIcon::ActivateLauncherIcon(arg);
    volume_->MountAndOpenInFileManager(arg.timestamp);
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
        volume_->MountAndOpenInFileManager(timestamp);
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
        volume_->MountAndOpenInFileManager(timestamp);
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
        volume_->EjectAndShowNotification();
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

  std::string GetRemoteUri()
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

  glib::SignalManager gsignals_;
  connection::Manager connections_;
};

//
// End private implementation
//

VolumeLauncherIcon::VolumeLauncherIcon(Volume::Ptr const& volume,
                                       DevicesSettings::Ptr const& devices_settings)
  : SimpleLauncherIcon(IconType::DEVICE)
  , pimpl_(new Impl(volume, devices_settings, this))
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
  pimpl_->ActivateLauncherIcon(arg);
}

AbstractLauncherIcon::MenuItemsVector VolumeLauncherIcon::GetMenus()
{
  return pimpl_->GetMenus();
}

std::string VolumeLauncherIcon::GetRemoteUri()
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

//
// Introspection
//
std::string VolumeLauncherIcon::GetName() const
{
  return "VolumeLauncherIcon";
}

} // namespace launcher
} // namespace unity
