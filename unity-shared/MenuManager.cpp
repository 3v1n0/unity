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
#include <UnityCore/GLibSource.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/DBusIndicators.h>
#include <unordered_map>

#include "MenuManager.h"
#include "InputMonitor.h"
#include "RawPixel.h"
#include "SigcSlotHash.h"
#include "UnitySettings.h"
#include "UScreen.h"
#include "WindowManager.h"

namespace unity
{
namespace menu
{
namespace
{
DECLARE_LOGGER(logger, "unity.menu.manager");

const std::string SETTINGS_NAME = "com.canonical.Unity";
const std::string LIM_KEY = "integrated-menus";
const std::string SHOW_MENUS_NOW_DELAY = "show-menus-now-delay";
const std::string ALWAYS_SHOW_MENUS_KEY = "always-show-menus";

const RawPixel TRIANGLE_THRESHOLD = 5_em;
const double SCRUB_VELOCITY_THRESHOLD = 0.05;
const unsigned MENU_OPEN_MOUSE_WAIT = 150;
}

using namespace indicator;

struct Manager::Impl : sigc::trackable
{
  Impl(Manager* parent, Indicators::Ptr const& indicators, key::Grabber::Ptr const& grabber)
    : parent_(parent)
    , indicators_(indicators)
    , key_grabber_(grabber)
    , show_now_window_(0)
    , last_pointer_time_(0)
    , settings_(g_settings_new(SETTINGS_NAME.c_str()))
  {
    for (auto const& indicator : indicators_->GetIndicators())
      AddIndicator(indicator);

    GrabMnemonicsForActiveWindow();

    parent_->show_menus.changed.connect(sigc::mem_fun(this, &Impl::ShowMenus));
    indicators_->on_object_added.connect(sigc::mem_fun(this, &Impl::AddIndicator));
    indicators_->on_object_removed.connect(sigc::mem_fun(this, &Impl::RemoveIndicator));
    indicators_->on_entry_activate_request.connect(sigc::mem_fun(this, &Impl::ActivateRequest));
    indicators_->on_entry_activated.connect(sigc::mem_fun(this, &Impl::EntryActivated));
    indicators_->icon_paths_changed.connect(sigc::mem_fun(this, &Impl::IconPathsChanged));
    WindowManager::Default().window_focus_changed.connect(sigc::hide(sigc::mem_fun(this, &Impl::GrabMnemonicsForActiveWindow)));

    signals_.Add<void, GSettings*, const gchar*>(settings_, "changed::" + LIM_KEY, [this] (GSettings*, const gchar*) {
      parent_->integrated_menus = g_settings_get_boolean(settings_, LIM_KEY.c_str());
    });
    signals_.Add<void, GSettings*, const gchar*>(settings_, "changed::" + SHOW_MENUS_NOW_DELAY, [this] (GSettings*, const gchar*) {
      parent_->show_menus_wait = g_settings_get_uint(settings_, SHOW_MENUS_NOW_DELAY.c_str());
    });
    signals_.Add<void, GSettings*, const gchar*>(settings_, "changed::" + ALWAYS_SHOW_MENUS_KEY, [this] (GSettings*, const gchar*) {
      parent_->always_show_menus = g_settings_get_boolean(settings_, ALWAYS_SHOW_MENUS_KEY.c_str());
    });

    parent_->integrated_menus = g_settings_get_boolean(settings_, LIM_KEY.c_str());
    parent_->show_menus_wait = g_settings_get_uint(settings_, SHOW_MENUS_NOW_DELAY.c_str());
    parent_->always_show_menus = g_settings_get_boolean(settings_, ALWAYS_SHOW_MENUS_KEY.c_str());
    parent_->menu_open = indicators_->GetActiveEntry() != nullptr;
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
      UngrabEntryMnemonics(entry);

    appmenu_.reset();
    parent_->appmenu_removed();
  }

  void GrabMnemonicsForActiveWindow()
  {
    if (!appmenu_)
      return;

    UngrabMnemonics();
    auto active_window = WindowManager::Default().GetActiveWindow();

    for (auto const& entry : appmenu_->GetEntriesForWindow(active_window))
      GrabEntryMnemonics(entry);
  }

  void GrabEntryMnemonics(indicator::Entry::Ptr const& entry)
  {
    if (entry->parent_window() != WindowManager::Default().GetActiveWindow())
      return;

    gunichar mnemonic;

    if (pango_parse_markup(entry->label().c_str(), -1, '_', nullptr, nullptr, &mnemonic, nullptr) && mnemonic)
    {
      auto key = gdk_keyval_to_lower(gdk_unicode_to_keyval(mnemonic));
      glib::String accelerator(gtk_accelerator_name(key, GDK_MOD1_MASK));
      auto const& id = entry->id();

      CompAction action;
      action.keyFromString(accelerator);
      action.setState(CompAction::StateInitKey);
      action.setInitiate([this, id] (CompAction* action, CompAction::State, CompOption::Vector&) {
        LOG_DEBUG(logger) << "pressed \"" << action->keyToString() << "\"";
        return parent_->key_activate_entry.emit(id);
      });

      if (uint32_t action_id = key_grabber_->AddAction(action))
        entry_actions_.insert({entry, action_id});
    }
  }

  void UngrabEntryMnemonics(indicator::Entry::Ptr const& entry)
  {
    auto it = entry_actions_.find(entry);

    if (it != entry_actions_.end())
    {
      key_grabber_->RemoveAction(it->second);
      entry_actions_.erase(it);
    }
  }

  void UngrabMnemonics()
  {
    for (auto it = entry_actions_.begin(); it != entry_actions_.end();)
    {
      key_grabber_->RemoveAction(it->second);
      it = entry_actions_.erase(it);
    }
  }

  void ActivateRequest(std::string const& entry_id)
  {
    parent_->key_activate_entry.emit(entry_id);
  }

  void EntryActivated(std::string const& menubar, std::string const&, nux::Rect const& geo)
  {
    parent_->menu_open = !geo.IsNull();

    if (active_menubar_ != menubar)
    {
      active_menubar_ = menubar;
      UpdateActiveTracker();
    }
  }

  void SetShowNowForWindow(Window xid, bool show)
  {
    if (!appmenu_)
      return;

    show_now_window_ = show ? xid : 0;

    for (auto const& entry : appmenu_->GetEntriesForWindow(xid))
      entry->set_show_now(show);
  }

  void ShowMenus(bool show)
  {
    if (!appmenu_)
      return;

    auto& wm = WindowManager::Default();

    if (show)
    {
      active_win_conn_ = wm.window_focus_changed.connect([this] (Window new_active) {
        SetShowNowForWindow(show_now_window_, false);
        SetShowNowForWindow(new_active, true);
      });
    }
    else
    {
      active_win_conn_->disconnect();
    }

    SetShowNowForWindow(wm.GetActiveWindow(), show);
  }

  void IconPathsChanged()
  {
    auto const& icon_paths = indicators_->IconPaths();
    std::vector<const gchar*> gicon_paths(icon_paths.size());

    for (auto const& path : icon_paths)
      gicon_paths.push_back(path.c_str());

    gtk_icon_theme_set_search_path(gtk_icon_theme_get_default(), gicon_paths.data(), gicon_paths.size());
  }

  bool PointInTriangle(nux::Point const& p, nux::Point const& t0, nux::Point const& t1, nux::Point const& t2)
  {
    int s = t0.y * t2.x - t0.x * t2.y + (t2.y - t0.y) * p.x + (t0.x - t2.x) * p.y;
    int t = t0.x * t1.y - t0.y * t1.x + (t0.y - t1.y) * p.x + (t1.x - t0.x) * p.y;

    if ((s < 0) != (t < 0))
      return false;

    int A = -t1.y * t2.x + t0.y * (t2.x - t1.x) + t0.x * (t1.y - t2.y) + t1.x * t2.y;
    if (A < 0)
    {
      s = -s;
      t = -t;
      A = -A;
    }

    return s > 0 && t > 0 && (s + t) < A;
  }

  double GetMouseVelocity(nux::Point const& p0, nux::Point const& p1, Time time_delta)
  {
    int dx, dy;
    double speed;

    if (time_delta == 0)
      return 1;

    dx = p0.x - p1.x;
    dy = p0.y - p1.y;

    speed = sqrt(dx * dx + dy * dy) / time_delta;

    return speed;
  }

  void OnActiveEntryEvent(XEvent const& e)
  {
    if (e.type != MotionNotify)
      return;

    auto const& active_entry = indicators_->GetActiveEntry();

    if (!active_entry)
      return;

    nux::Point mouse(e.xmotion.x_root, e.xmotion.y_root);
    auto monitor = UScreen::GetDefault()->GetMonitorAtPosition(mouse.x, mouse.y);
    double scale = Settings::Instance().em(monitor)->DPIScale();
    double speed = GetMouseVelocity(mouse, tracked_pointer_pos_, e.xmotion.time - last_pointer_time_);
    auto menu_geo = active_entry->geometry();

    tracked_pointer_pos_ = mouse;
    last_pointer_time_ = e.xmotion.time;

    if (speed > SCRUB_VELOCITY_THRESHOLD &&
        PointInTriangle(mouse, {mouse.x, std::max(mouse.y - TRIANGLE_THRESHOLD.CP(scale), 0)},
                        menu_geo.GetPosition(), {menu_geo.x + menu_geo.width, menu_geo.y}))
    {
      pointer_movement_timeout_ = std::make_shared<glib::Timeout>(MENU_OPEN_MOUSE_WAIT, [this, mouse, speed] {
        if (active_tracker_)
          active_tracker_(mouse.x, mouse.y, speed);

        return false;
      });

      return;
    }

    if (active_tracker_)
    {
      pointer_movement_timeout_.reset();
      active_tracker_(mouse.x, mouse.y, speed);
    }
  }

  bool RegisterTracker(std::string const& menubar, PositionTracker const& cb)
  {
    bool added = position_trackers_.insert({menubar, cb}).second;

    if (added && active_menubar_ == menubar)
      UpdateActiveTracker();

    return added;
  }

  bool UnregisterTracker(std::string const& menubar, PositionTracker const& cb)
  {
    auto it = position_trackers_.find(menubar);

    if (it == end(position_trackers_))
      return false;

    if (!cb || (cb && it->second == cb))
    {
      position_trackers_.erase(it);
      UpdateActiveTracker();
      return true;
    }

    return false;
  }

  void UpdateActiveTracker()
  {
    auto it = position_trackers_.find(active_menubar_);
    active_tracker_ = (it != end(position_trackers_)) ? it->second : PositionTracker();
    pointer_movement_timeout_.reset();

    if (active_tracker_)
    {
      if (input::Monitor::Get().RegisterClient(input::Events::POINTER, sigc::mem_fun(this, &Impl::OnActiveEntryEvent)))
        last_pointer_time_ = 0;
    }
    else
    {
      input::Monitor::Get().UnregisterClient(sigc::mem_fun(this, &Impl::OnActiveEntryEvent));

      if (it != end(position_trackers_))
        position_trackers_.erase(it);
    }
  }

  Manager* parent_;
  Indicators::Ptr indicators_;
  AppmenuIndicator::Ptr appmenu_;
  key::Grabber::Ptr key_grabber_;
  Window show_now_window_;
  std::string active_menubar_;
  PositionTracker active_tracker_;
  nux::Point tracked_pointer_pos_;
  Time last_pointer_time_;
  glib::Source::Ptr pointer_movement_timeout_;
  connection::Manager appmenu_connections_;
  connection::Wrapper active_win_conn_;
  glib::Object<GSettings> settings_;
  glib::SignalManager signals_;
  std::unordered_map<std::string, PositionTracker> position_trackers_;
  std::unordered_map<indicator::Entry::Ptr, uint32_t> entry_actions_;
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

bool Manager::RegisterTracker(std::string const& menubar, PositionTracker const& cb)
{
  return impl_->RegisterTracker(menubar, cb);
}

bool Manager::UnregisterTracker(std::string const& menubar, PositionTracker const& cb)
{
  return impl_->UnregisterTracker(menubar, cb);
}


} // menu namespace
} // unity namespace
