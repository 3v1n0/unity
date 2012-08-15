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
  Impl(std::shared_ptr<Volume> const& volume,
       std::shared_ptr<DevicesSettings> const& devices_settings,
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

    parent_->SetIconType(IconType::DEVICE);
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
    volume_changed_connection_ = volume_->changed.connect([this]() {
      // FIXME: find a way to remove this timeout.
      changed_timeout_.reset(new glib::Timeout(volume_changed_timeout, [this]() {
        UpdateIcon();
        UpdateVisibility();
        return false;
      }));
    });

    devices_settings_->changed.connect([this]() {
      UpdateVisibility();
    });
  }

  bool CanEject() const
  {
    return volume_->CanBeEjected();
  }

  void EjectAndShowNotification()
  {
    return volume_->EjectAndShowNotification();
  }

  void OnRemoved()
  {
    volume_changed_connection_.disconnect();
    changed_timeout_.reset();

    if (devices_settings_->IsABlacklistedDevice(volume_->GetIdentifier()))
      devices_settings_->RemoveBlacklisted(volume_->GetIdentifier());

    parent_->Remove();
  }

  void ActivateLauncherIcon(ActionArg arg)
  {
    parent_->SimpleLauncherIcon::ActivateLauncherIcon(arg);
    parent_->SetQuirk(Quirk::STARTING, true);

    volume_->MountAndOpenInFileManager();
  }

  std::list<DbusmenuMenuitem*> GetMenus()
  {
    std::list<DbusmenuMenuitem*> result;

    AppendUnlockFromLauncherItem(result);
    AppendOpenItem(result);
    AppendEjectItem(result);
    AppendSafelyRemoveItem(result);
    AppendUnmountItem(result);

    return result;
  }

  void AppendUnlockFromLauncherItem(std::list<DbusmenuMenuitem*>& menu)
  {
    DbusmenuMenuitem* menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unlock from Launcher"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    unlock_menuitem_activated_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        auto identifier = volume_->GetIdentifier();
        devices_settings_->AddBlacklisted(identifier);
        UpdateVisibility();
    });

    menu.push_back(menu_item);
  }

  void AppendOpenItem(std::list<DbusmenuMenuitem*>& menu)
  {
    DbusmenuMenuitem* menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Open"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    open_menuitem_activated_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->MountAndOpenInFileManager();
    });

    menu.push_back(menu_item);
  }

  void AppendEjectItem(std::list<DbusmenuMenuitem*>& menu)
  {
    if (!volume_->CanBeEjected())
      return;

    DbusmenuMenuitem* menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, volume_->HasSiblings() ? _("Eject parent drive") : _("Eject"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    eject_menuitem_activated_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->EjectAndShowNotification();
    });

    menu.push_back(menu_item);
  }

  void AppendSafelyRemoveItem(std::list<DbusmenuMenuitem*>& menu)
  {
    if (!volume_->CanBeStopped())
      return;

    DbusmenuMenuitem* menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, volume_->HasSiblings() ? _("Safely remove parent drive") : _("Safely remove"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    stop_menuitem_activated_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->StopDrive();
    });

    menu.push_back(menu_item);
  }

  void AppendUnmountItem(std::list<DbusmenuMenuitem*>& menu)
  {
    if (volume_->CanBeEjected() || volume_->CanBeStopped())
      return;

    DbusmenuMenuitem* menu_item = dbusmenu_menuitem_new();

    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Unmount"));
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);

    unmount_menuitem_activated_.Connect(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, [this] (DbusmenuMenuitem*, int) {
        volume_->Unmount();
    });

    menu.push_back(menu_item);
  }

  VolumeLauncherIcon* parent_;
  bool keep_in_launcher_;
  std::shared_ptr<Volume> volume_;
  std::shared_ptr<DevicesSettings> devices_settings_;

  sigc::connection volume_changed_connection_;
  glib::Source::UniquePtr changed_timeout_;
  glib::Signal<void, DbusmenuMenuitem*, int> unlock_menuitem_activated_;
  glib::Signal<void, DbusmenuMenuitem*, int> open_menuitem_activated_;
  glib::Signal<void, DbusmenuMenuitem*, int> eject_menuitem_activated_;
  glib::Signal<void, DbusmenuMenuitem*, int> stop_menuitem_activated_;
  glib::Signal<void, DbusmenuMenuitem*, int> unmount_menuitem_activated_;
};

//
// End private implementation
//

VolumeLauncherIcon::VolumeLauncherIcon(std::shared_ptr<Volume> const& volume,
                                       std::shared_ptr<DevicesSettings> const& devices_settings)
  : pimpl_(new Impl(volume, devices_settings, this))
{}

VolumeLauncherIcon::~VolumeLauncherIcon()
{}

bool VolumeLauncherIcon::CanEject()
{
  return pimpl_->CanEject();
}

void VolumeLauncherIcon::EjectAndShowNotification()
{
  pimpl_->EjectAndShowNotification();
}

void VolumeLauncherIcon::OnRemoved()
{
  pimpl_->OnRemoved();
}

void VolumeLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  pimpl_->ActivateLauncherIcon(arg);
}

std::list<DbusmenuMenuitem*> VolumeLauncherIcon::GetMenus()
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
