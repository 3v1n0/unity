// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 */

#ifndef UNITY_FILESYSTEM_LENSES_H
#define UNITY_FILESYSTEM_LENSES_H

#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

#include "Lenses.h"

namespace unity
{
namespace dash
{

class LensDirectoryReader : public sigc::trackable
{
public:
  struct LensFileData
  {
    LensFileData(GKeyFile* file, const gchar *lens_id);
    static bool IsValid(GKeyFile* file, glib::Error& error);

    glib::String id;
    glib::String domain;
    glib::String dbus_name;
    glib::String dbus_path;
    glib::String name;
    glib::String icon;
    glib::String description;
    glib::String search_hint;
    bool         visible;
    glib::String shortcut;
  };

  typedef std::shared_ptr<LensDirectoryReader> Ptr;
  typedef std::shared_ptr<LensFileData> LensFileDataPtr;
  typedef std::vector<LensFileDataPtr> DataList;

  LensDirectoryReader(std::string const& directory);
  ~LensDirectoryReader();

  static LensDirectoryReader::Ptr GetDefault();

  bool IsDataLoaded() const;
  DataList GetLensData() const;

  sigc::signal<void> load_finished;

private:
  class Impl;
  Impl* pimpl;
};

// Reads Lens information from the filesystem, as per-specification, and creates
// Lens instances using this data
class FilesystemLenses : public Lenses
{
public:
  typedef std::shared_ptr<FilesystemLenses> Ptr;

  FilesystemLenses();
  FilesystemLenses(LensDirectoryReader::Ptr const& reader);

  ~FilesystemLenses();

  LensList GetLenses() const override;
  Lens::Ptr GetLens(std::string const& lens_id) const override;
  Lens::Ptr GetLensAtIndex(std::size_t index) const override;
  Lens::Ptr GetLensForShortcut(std::string const& lens_shortcut) const override;

  sigc::signal<void> lenses_loaded;

private:
  void Init();

  class Impl;
  Impl* pimpl;
};

}
}

#endif
