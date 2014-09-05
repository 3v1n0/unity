// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "LockScreenSettings.h"

#include <NuxCore/Logger.h>
#include <sigc++/adaptors/hide.h>
#include <UnityCore/GLibSignal.h>
#include <UnityCore/GLibWrapper.h>

namespace unity
{
namespace lockscreen
{

DECLARE_LOGGER(logger, "unity.lockscreen.settings");

namespace
{
Settings* settings_instance = nullptr;

const std::string GREETER_SETTINGS = "com.canonical.unity-greeter";
const std::string LOGO_KEY = "logo";
const std::string FONT_KEY = "font-name";
const std::string BACKGROUND_KEY = "background";
const std::string BACKGROUND_COLOR_KEY = "background-color";
const std::string USER_BG_KEY = "draw-user-backgrounds";
const std::string DRAW_GRID_KEY = "draw-grid";
const std::string SHOW_HOSTNAME_KEY = "show-hostname";

const std::string GS_SETTINGS = "org.gnome.desktop.screensaver";
const std::string IDLE_ACTIVATION_ENABLED_KEY = "idle-activation-enabled";
const std::string LOCK_DELAY = "lock-delay";
const std::string LOCK_ENABLED = "lock-enabled";
const std::string LOCK_ON_SUSPEND = "ubuntu-lock-on-suspend";

const std::string A11Y_SETTINGS = "org.gnome.desktop.a11y.applications";
const std::string USE_SCREEN_READER = "screen-reader-enabled";
const std::string USE_OSK = "screen-keyboard-enabled";
}

const RawPixel Settings::GRID_SIZE = 40_em;

struct Settings::Impl
{
  Impl()
    : greeter_settings_(g_settings_new(GREETER_SETTINGS.c_str()))
    , gs_settings_(g_settings_new(GS_SETTINGS.c_str()))
    , a11y_settings_(g_settings_new(A11Y_SETTINGS.c_str()))
    , greeter_signal_(greeter_settings_, "changed", sigc::hide(sigc::hide(sigc::mem_fun(this, &Impl::UpdateGreeterSettings))))
    , gs_signal_(gs_settings_, "changed", sigc::hide(sigc::hide(sigc::mem_fun(this, &Impl::UpdateGSSettings))))
    , osk_signal_(a11y_settings_, "changed", sigc::hide(sigc::hide(sigc::mem_fun(this, &Impl::UpdateA11YSettings))))
  {
    UpdateGreeterSettings();
    UpdateGSSettings();
    UpdateA11YSettings();
  }

  void UpdateGreeterSettings()
  {
    auto* s = settings_instance;
    s->font_name = glib::String(g_settings_get_string(greeter_settings_, FONT_KEY.c_str())).Str();
    s->logo = glib::String(g_settings_get_string(greeter_settings_, LOGO_KEY.c_str())).Str();
    s->background = glib::String(g_settings_get_string(greeter_settings_, BACKGROUND_KEY.c_str())).Str();
    s->background_color = nux::Color(glib::String(g_settings_get_string(greeter_settings_, BACKGROUND_COLOR_KEY.c_str())).Str());
    s->show_hostname = g_settings_get_boolean(greeter_settings_, SHOW_HOSTNAME_KEY.c_str()) != FALSE;
    s->use_user_background = g_settings_get_boolean(greeter_settings_, USER_BG_KEY.c_str()) != FALSE;
    s->draw_grid = g_settings_get_boolean(greeter_settings_, DRAW_GRID_KEY.c_str()) != FALSE;
  }

  void UpdateGSSettings()
  {
    auto* s = settings_instance;
    s->lock_on_blank = g_settings_get_boolean(gs_settings_, LOCK_ENABLED.c_str()) != FALSE;
    s->lock_on_suspend = g_settings_get_boolean(gs_settings_, LOCK_ON_SUSPEND.c_str()) != FALSE;
    s->lock_delay = g_settings_get_uint(gs_settings_, LOCK_DELAY.c_str());
  }

  void UpdateA11YSettings()
  {
    bool legacy = false;
    legacy = g_settings_get_boolean(a11y_settings_, USE_SCREEN_READER.c_str()) != FALSE;
    legacy = legacy || g_settings_get_boolean(a11y_settings_, USE_OSK.c_str()) != FALSE;
    settings_instance->use_legacy = legacy;
  }

  glib::Object<GSettings> greeter_settings_;
  glib::Object<GSettings> gs_settings_;
  glib::Object<GSettings> a11y_settings_;

  glib::Signal<void, GSettings*, const gchar*> greeter_signal_;
  glib::Signal<void, GSettings*, const gchar*> gs_signal_;
  glib::Signal<void, GSettings*, const gchar*> osk_signal_;
};

Settings::Settings()
{
  if (settings_instance)
  {
    LOG_ERROR(logger) << "More than one lockscreen::Settings created.";
  }
  else
  {
    settings_instance = this;
    impl_.reset(new Impl());
  }
}

Settings::~Settings()
{
  if (settings_instance == this)
    settings_instance = nullptr;
}

Settings& Settings::Instance()
{
  if (!settings_instance)
  {
    LOG_ERROR(logger) << "No lockscreen::Settings created yet.";
  }

  return *settings_instance;
}

}
}
