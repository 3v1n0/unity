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
}

struct Settings::Impl
{
  Impl()
    : greeter_settings_(g_settings_new(GREETER_SETTINGS.c_str()))
    , signal_(greeter_settings_, "changed", sigc::hide(sigc::hide(sigc::mem_fun(this, &Impl::UpdateSettings))))
  {
    UpdateSettings();
  }

  void UpdateSettings()
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

  glib::Object<GSettings> greeter_settings_;
  glib::Signal<void, GSettings*, const gchar*> signal_;
};

Settings::Settings()
{
  if (settings_instance)
  {
    LOG_ERROR(logger) << "More than one lockscreen::Settings created.";
  }
  else
  {
    lockscreen_type = Type::UNITY;
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
