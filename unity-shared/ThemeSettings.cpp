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

#include <NuxCore/Logger.h>
#include <UnityCore/DesktopUtilities.h>
#include <UnityCore/ConnectionManager.h>
#include "FontSettings.h"

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
    , theme_setting_("gtk-theme-name")
    , font_setting_("gtk-font-name")
  {
    parent_->theme = theme_setting_();
    parent_->font = font_setting_();

    connections_.Add(theme_setting_.changed.connect([this] (std::string const& theme) {
      parent_->theme = theme;
      LOG_INFO(logger) << "gtk-theme-name changed to " << parent_->theme();
    }));

    connections_.Add(font_setting_.changed.connect([this] (std::string const& font) {
      parent_->font = font;
      LOG_INFO(logger) << "gtk-font-name changed to " << parent_->font();
    }));
  }

  std::string ThemedFilePath(std::string const& base_filename, std::vector<std::string> const& extra_folders, std::vector<std::string> extensions) const
  {
    auto const& theme = parent_->theme();
    auto const& home_dir = DesktopUtilities::GetUserHomeDirectory();
    auto const& data_dir = DesktopUtilities::GetUserDataDirectory();
    const char* gtk_prefix = g_getenv("GTK_DATA_PREFIX");

    if (!gtk_prefix || gtk_prefix[0] == '\0')
      gtk_prefix = GTK_PREFIX;

    extensions.insert(end(extensions), begin(THEMED_FILE_EXTENSIONS), end(THEMED_FILE_EXTENSIONS));

    for (auto const& extension : extensions)
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
  FontSettings font_settings_;
  gtk::Setting<std::string> theme_setting_;
  gtk::Setting<std::string> font_setting_;
  connection::Manager connections_;
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

std::string Settings::ThemedFilePath(std::string const& basename, std::vector<std::string> const& extra_folders, std::vector<std::string> const& extra_extensions) const
{
  return impl_->ThemedFilePath(basename, extra_folders, extra_extensions);
}

} // theme namespace
} // unity namespace
