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

#ifndef UNITY_THEME_SETTINGS
#define UNITY_THEME_SETTINGS

#include <memory>
#include <NuxCore/Property.h>

struct _GtkIconTheme;

namespace unity
{
namespace theme
{

class Settings
{
public:
  typedef std::shared_ptr<Settings> Ptr;

  static Settings::Ptr const& Get();
  ~Settings();

  nux::Property<std::string> theme;
  nux::Property<std::string> font;

  std::string ThemedFilePath(std::string const& basename, std::vector<std::string> const& extra_folders = {}, std::vector<std::string> const& extra_extensions = {}) const;
  _GtkIconTheme* UnityIconTheme() const;

  sigc::signal<void> icons_changed;

private:
  Settings();
  Settings(Settings const&) = delete;
  Settings& operator=(Settings const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // theme namespace
} // unity namespace

#endif // UNITY_THEME_SETTINGS
