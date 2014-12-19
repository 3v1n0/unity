// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *              William Hua <william.hua@canonical.com>
 */

#include <gtk/gtk.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DBusIndicators.h>
#include <unordered_map>

#include "MenuManager.h"

namespace unity
{
namespace menu
{
namespace
{
DECLARE_LOGGER(logger, "unity.menu.manager");

const std::string SETTINGS_NAME = "com.canonical.Unity";
const std::string LIM_KEY = "integrated-menus";
const std::string ALWAYS_SHOW_MENUS_KEY = "always-show-menus";
}

using namespace indicator;

struct Manager::Impl : sigc::trackable
{
  Impl(Manager* parent, Indicators::Ptr const& indicators, key::Grabber::Ptr const& grabber)
    : parent_(parent)
    , indicators_(indicators)
    , key_grabber_(grabber)
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
  {
    for (auto const& indicator : indicators_->GetIndicators())
      AddIndicator(indicator);

    parent_->show_menus.changed.connect(sigc::mem_fun(this, &Impl::ShowMenus));
    indicators_->on_object_added.connect(sigc::mem_fun(this, &Impl::AddIndicator));
    indicators_->on_object_removed.connect(sigc::mem_fun(this, &Impl::RemoveIndicator));
    indicators_->on_entry_activate_request.connect(sigc::mem_fun(this, &Impl::ActivateRequest));
    indicators_->icon_paths_changed.connect(sigc::mem_fun(this, &Impl::IconPathsChanged));

    parent_->integrated_menus = g_settings_get_boolean(settings_, LIM_KEY.c_str());
    parent_->always_show_menus = g_settings_get_boolean(settings_, ALWAYS_SHOW_MENUS_KEY.c_str());

    signals_.Add<void, GSettings*, const gchar*>(settings_, "changed::" + LIM_KEY, [this] (GSettings*, const gchar*) {
      parent_->integrated_menus = g_settings_get_boolean(settings_, LIM_KEY.c_str());
    });
    signals_.Add<void, GSettings*, const gchar*>(settings_, "changed::" + ALWAYS_SHOW_MENUS_KEY, [this] (GSettings*, const gchar*) {
      parent_->always_show_menus = g_settings_get_boolean(settings_, ALWAYS_SHOW_MENUS_KEY.c_str());
    });
  }

  ~Impl()
  {
    if (appmenu_)
      RemoveIndicator(appmenu_);
  }

  void AddIndicator(Indicator::Ptr const& indicator)
  {
    if (!indicator->IsAppmenu())
      return;

    appmenu_connections_.Clear();
    appmenu_ = std::static_pointer_cast<AppmenuIndicator>(indicator);

    for (auto const& entry : appmenu_->GetEntries())
      GrabEntryMnemonics(entry);

    appmenu_connections_.Add(appmenu_->on_entry_added.connect(sigc::mem_fun(this, &Impl::GrabEntryMnemonics)));
    appmenu_connections_.Add(appmenu_->on_entry_removed.connect(sigc::mem_fun(this, &Impl::UngrabEntryMnemonics)));

    parent_->appmenu_added();
  }

  void RemoveIndicator(Indicator::Ptr const& indicator)
  {
    if (indicator != appmenu_)
      return;

    appmenu_connections_.Clear();

    for (auto const& entry : appmenu_->GetEntries())
      UngrabEntryMnemonics(entry->id());

    appmenu_.reset();
    parent_->appmenu_removed();
  }

  void GrabEntryMnemonics(indicator::Entry::Ptr const& entry)
  {
    gunichar mnemonic;

    if (pango_parse_markup(entry->label().c_str(), -1, '_', nullptr, nullptr, &mnemonic, nullptr) && mnemonic)
    {
      auto key = gdk_keyval_to_lower(gdk_unicode_to_keyval(mnemonic));
      glib::String accelerator(gtk_accelerator_name(key, GDK_MOD1_MASK));
      auto action = std::make_shared<CompAction>();
      auto id = entry->id();

      action->keyFromString(accelerator);
      action->setState(CompAction::StateInitKey);
      action->setInitiate([this, id] (CompAction* action, CompAction::State, CompOption::Vector&) {
        LOG_DEBUG(logger) << "pressed \"" << action->keyToString() << "\"";
        return parent_->key_activate_entry.emit(id);
      });

      entry_actions_[id] = action;
      key_grabber_->AddAction(*action);
    }
  }

  void UngrabEntryMnemonics(std::string const& entry_id)
  {
    auto it = entry_actions_.find(entry_id);

    if (it != entry_actions_.end())
    {
      key_grabber_->RemoveAction(*it->second);
      entry_actions_.erase(it);
    }
  }

  void ActivateRequest(std::string const& entry_id)
  {
    parent_->key_activate_entry.emit(entry_id);
  }

  void ShowMenus(bool show)
  {
    if (!appmenu_)
      return;

    for (auto const& entry : appmenu_->GetEntries())
      entry->set_show_now(show);
  }

  void IconPathsChanged()
  {
    auto const& icon_paths = indicators_->IconPaths();
    std::vector<const gchar*> gicon_paths(icon_paths.size());

    for (auto const& path : icon_paths)
      gicon_paths.push_back(path.c_str());

    gtk_icon_theme_set_search_path(gtk_icon_theme_get_default(), gicon_paths.data(), gicon_paths.size());
  }

  Manager* parent_;
  Indicators::Ptr indicators_;
  AppmenuIndicator::Ptr appmenu_;
  key::Grabber::Ptr key_grabber_;
  connection::Manager appmenu_connections_;
  glib::Object<GSettings> settings_;
  glib::SignalManager signals_;
  std::unordered_map<std::string, std::shared_ptr<CompAction>> entry_actions_;
};

Manager::Manager(Indicators::Ptr const& indicators, key::Grabber::Ptr const& grabber)
  : integrated_menus(false)
  , show_menus_wait(180)
  , always_show_menus(false)
  , fadein(100)
  , fadeout(120)
  , discovery(2)
  , discovery_fadein(200)
  , discovery_fadeout(300)
  , impl_(new Impl(this, indicators, grabber))
{}

Manager::~Manager()
{}

bool Manager::HasAppMenu() const
{
  return impl_->appmenu_ && !impl_->appmenu_->GetEntries().empty();
}

Indicators::Ptr const& Manager::Indicators() const
{
  return impl_->indicators_;
}

AppmenuIndicator::Ptr const& Manager::AppMenu() const
{
  return impl_->appmenu_;
}

key::Grabber::Ptr const& Manager::KeyGrabber() const
{
  return impl_->key_grabber_;
}

} // menu namespace
} // unity namespace
