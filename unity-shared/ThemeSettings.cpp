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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include "ThemeSettings.h"

#include <gtk/gtk.h>
#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

namespace unity
{
namespace theme
{
namespace
{
DECLARE_LOGGER(logger, "unity.theme.settings");

const std::array<std::string, 2> THEMED_FILE_EXTENSIONS = { "svg", "png" };
}

struct Settings::Impl
{
  Impl(Settings* parent)
    : parent_(parent)
  {
    parent_->theme = glib::String(GetSettingValue<gchar*>("gtk-theme-name")).Str();
    parent_->font = glib::String(GetSettingValue<gchar*>("gtk-font-name")).Str();

    GtkSettings* settings = gtk_settings_get_default();
    signals_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-theme-name", [this] (GtkSettings*, GParamSpec*) {
      parent_->theme = glib::String(GetSettingValue<gchar*>("gtk-theme-name")).Str();
      LOG_INFO(logger) << "gtk-theme-name changed to " << parent_->theme();
    });

    signals_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-font-name", [this] (GtkSettings*, GParamSpec*) {
      parent_->font = glib::String(GetSettingValue<gchar*>("gtk-font-name")).Str();
      LOG_INFO(logger) << "gtk-font-name changed to " << parent_->font();
    });
  }

  template <typename TYPE>
  inline TYPE GetSettingValue(std::string const& name)
  {
    TYPE value;
    g_object_get(gtk_settings_get_default(), name.c_str(), &value, nullptr);
    return value;
  }

  std::string ThemedFilePath(std::string const& base_filename, std::vector<std::string> const& extra_folders = {}) const
  {
    auto const& theme = parent_->theme();
    auto const& home_dir = DesktopUtilities::GetUserHomeDirectory();
    auto const& data_dir = DesktopUtilities::GetUserDataDirectory();
    const char* gtk_prefix = g_getenv("GTK_DATA_PREFIX");

    if (!gtk_prefix || gtk_prefix[0] == '\0')
      gtk_prefix = GTK_PREFIX;

    for (auto const& extension : THEMED_FILE_EXTENSIONS)
    {
      auto filename = base_filename + '.' + extension;
      glib::String subpath(g_build_filename(theme.c_str(), "unity", filename.c_str(), nullptr));

      glib::String local_file(g_build_filename(data_dir.c_str(), "themes", subpath.Value(), nullptr));

      if (g_file_test(local_file, G_FILE_TEST_EXISTS))
        return local_file.Str();

      glib::String home_file(g_build_filename(home_dir.c_str(), ".themes", subpath.Value(), nullptr));

      if (g_file_test(home_file, G_FILE_TEST_EXISTS))
        return home_file.Str();

      glib::String theme_file(g_build_filename(gtk_prefix, "share", "themes", subpath.Value(), nullptr));

      if (g_file_test(theme_file, G_FILE_TEST_EXISTS))
        return theme_file.Str();

      for (auto const& folder : extra_folders)
      {
        glib::String path(g_build_filename(folder.c_str(), filename.c_str(), nullptr));

        if (g_file_test(path, G_FILE_TEST_EXISTS))
          return path.Str();
      }
    }

    return std::string();
  }

  Settings* parent_;
  glib::SignalManager signals_;
};

Settings::Ptr const& Settings::Get()
{
  static Settings::Ptr theme(new Settings());
  return theme;
}

Settings::Settings()
  : impl_(new Impl(this))
{}

Settings::~Settings()
{}

std::string Settings::ThemedFilePath(std::string const& basename, std::vector<std::string> const& extra_folders) const
{
  return impl_->ThemedFilePath(basename, extra_folders);
}

} // theme namespace
} // unity namespace
