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


#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibSignal.h>

#include "DevicesSettings.h"
#include "Volume.h"
#include "VolumeLauncherIcon.h"

namespace unity
{
namespace launcher
{
namespace
{

nux::logging::Logger logger("unity.launcher");

const unsigned int volume_changed_timeout =  500;

}

//
// Start private implementation
//
class VolumeLauncherIcon::Impl
{
public:
  typedef glib::Signal<void, DbusmenuMenuitem*, int> ItemSignal;

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

  ~Impl()
  { 
    volume_changed_conn_.disconnect();
    settings_changed_conn_.disconnect();
  }

  void UpdateIcon()
  {
    parent_->tooltip_text = volume_->GetName();
    parent_->icon_name = volume_->GetIconName();

    parent_->SetQuirk(Quirk::RUNNING, false);
  }

  void UpdateVisibility()
  {
    UpdateKeepInLauncher();
    parent_->SetQuirk(Quirk::VISIBLE, keep_in_launcher_);
  }

  void UpdateKeepInLauncher()
  {
    auto identifier = volume_->GetIdentifier();
    keep_in_launcher_ = !devices_settings_->IsABlacklistedDevice(identifier);
  }

  void ConnectSignals()
  {
    volume_changed_conn_ = volume_->changed.connect(sigc::mem_fun(this, &Impl::OnVolumeChanged));
    settings_changed_conn_ = devices_settings_->changed.connect(sigc::mem_fun(this, &Impl::OnSettingsChanged));
  }

  void OnVolumeChanged()
  {
    UpdateIcon();
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

  void OnRemoved()
  {
    if (devices_settings_->IsABlacklistedDevice(volume_->GetIdentifier()))
      devices_settings_->TryToUnblacklist(volume_->GetIdentifier());

    parent_->Remove();
  }

  void ActivateLauncherIcon(ActionArg arg)
  {
    parent_->SimpleLauncherIcon::ActivateLauncherIcon(arg);
    volume_->MountAndOpenInFileManager();
  }

  MenuItemsVector GetMenus()
  {
    MenuItemsVector result;

    AppendUnlockFromLauncherItem(result);
    AppendOpenItem(result);
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
        auto identifier = volume_->GetIdentifier();
        devices_settings_->TryToBlacklist(identifier);
    }));

    menu.push_back(menu_item);
  }

  void AppendOpenItem(MenuItemsVector& menu)
  {
    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    gsignals_.Add(new ItemSignal(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->MountAndOpenInFileManager();
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

  VolumeLauncherIcon* parent_;
  bool keep_in_launcher_;
  Volume::Ptr volume_;
  DevicesSettings::Ptr devices_settings_;

  glib::SignalManager gsignals_;
  sigc::connection settings_changed_conn_;
  sigc::connection volume_changed_conn_;
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

void VolumeLauncherIcon::OnRemoved()
{
  pimpl_->OnRemoved();
}

void VolumeLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  pimpl_->ActivateLauncherIcon(arg);
}

AbstractLauncherIcon::MenuItemsVector VolumeLauncherIcon::GetMenus()
{
  return pimpl_->GetMenus();
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
