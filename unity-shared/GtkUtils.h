// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2016 Canonical Ltd
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
 */

#ifndef UNITY_GTK_UTILS
#define UNITY_GTK_UTILS

#include <gtk/gtk.h>
#include <sigc++/sigc++.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace gtk
{

template <typename TYPE>
inline TYPE GetSettingValue(std::string const& name)
{
  TYPE value;
  g_object_get(gtk_settings_get_default(), name.c_str(), &value, nullptr);
  return value;
}

template <>
inline std::string GetSettingValue(std::string const& name)
{
  return glib::String(gtk::GetSettingValue<gchar*>(name)).Str();
}

template <typename TYPE>
class Setting
{
public:
  Setting(std::string const& name)
    : name_(name)
  {
    connection_.Connect(gtk_settings_get_default(), "notify::"+name_, [this] (GtkSettings*, GParamSpec*) {
      changed.emit(GetSettingValue<TYPE>(name_));
    });
  }

  TYPE Value() const { return GetSettingValue<TYPE>(name_); }
  TYPE operator()() const { return Value(); }

  sigc::signal<void, TYPE const&> changed;

private:
  std::string name_;
  glib::Signal<void, GtkSettings*, GParamSpec*> connection_;
};

} // theme namespace
} // gtk namespace

#endif // UNITY_GTK_UTILS
