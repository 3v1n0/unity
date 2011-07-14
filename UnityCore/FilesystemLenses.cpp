// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#include "FilesystemLenses.h"

#include <gio/gio.h>
#include <glib.h>
#include <NuxCore/Logger.h>
#include <vector>

#include "config.h"
#include "GLibWrapper.h"


namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.filesystemlenses");
}

class FilesystemLenses::Impl
{
public:
  typedef std::vector<glib::Object<GFile>> Directories;
  typedef std::vector<std::string> LensIDs;

  Impl(FilesystemLenses* owner);
  Impl(FilesystemLenses* owner, std::vector<std::string> const& lens_directories);

  ~Impl();

  LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(unsigned int index) const;
  unsigned int LensCount() const;

  void Init();
  glib::Object<GFile> BuildLensPathFileWithSuffix(std::string const& directory);
  void LoadLensesFromDirectory(glib::Object<GFile>& directory); 
  static void OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl *self);
  void EnumerateLensesDirectoryChildren(GFileEnumerator* enumerator);

  FilesystemLenses* owner_;
  Directories directories_;
  LensIDs lens_ids_;
};

FilesystemLenses::Impl::Impl(FilesystemLenses* owner)
  : owner_(owner)
{
  LOG_DEBUG(logger) << "Initialising in standard directories mode";

  // Add all Lens file search paths in order of system -> local
  directories_.push_back(BuildLensPathFileWithSuffix(g_get_user_data_dir()));
  directories_.push_back(BuildLensPathFileWithSuffix(DATADIR));
  directories_.push_back(BuildLensPathFileWithSuffix("/usr/local/share"));
  directories_.push_back(BuildLensPathFileWithSuffix("/usr/share"));

  Init();
}

FilesystemLenses::Impl::Impl(FilesystemLenses* owner, std::vector<std::string> const& lens_directories)
  : owner_(owner)
{
  LOG_DEBUG(logger) << "Initialising in override directories mode";

  for (std::string const& directory: lens_directories)
  {
    glib::Object<GFile> file(g_file_new_for_path(directory.c_str()));
    directories_.push_back(file);
  }

  Init();
}

FilesystemLenses::Impl::~Impl()
{}

void FilesystemLenses::Impl::Init()
{
  // Start reading the directories
  for (glib::Object<GFile> &directory: directories_)
  {
    glib::String path(g_file_get_path(directory));
    LOG_DEBUG(logger) << "Searching for Lenses in: " << path.Str();

    LoadLensesFromDirectory(directory);
  }
}

glib::Object<GFile> FilesystemLenses::Impl::BuildLensPathFileWithSuffix(std::string const& directory)
{
  glib::String ret(g_build_filename(directory.c_str(), "unity", "lenses", NULL));
  glib::Object<GFile> file(g_file_new_for_path (ret.Value()));
  return file;
}

void FilesystemLenses::Impl::LoadLensesFromDirectory(glib::Object<GFile>& directory)
{
  g_file_enumerate_children_async(directory,
                                  G_FILE_ATTRIBUTE_STANDARD_NAME,
                                  G_FILE_QUERY_INFO_NONE,
                                  G_PRIORITY_DEFAULT,
                                  NULL,
                                  (GAsyncReadyCallback)OnDirectoryEnumerated,
                                  this);

}

void FilesystemLenses::Impl::OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl *self)
{
  glib::Error error;
  glib::Object<GFileEnumerator> enumerator(g_file_enumerate_children_finish(source, res, error.AsOutParam()));

  if (error || !enumerator)
  {
    glib::String path(g_file_get_path(source));
    LOG_WARN(logger) << "Unabled to enumerate children of directory "
                      << path.Str() << " "
                      << error.Message();
    return;
  }
  self->EnumerateLensesDirectoryChildren(enumerator);
}

void FilesystemLenses::Impl::EnumerateLensesDirectoryChildren(GFileEnumerator* enumerator)
{
  glib::Error error;
  glib::Object<GFileInfo> info;

  while (info = g_file_enumerator_next_file(enumerator, NULL, error.AsOutParam()))
  {
    if (error || !info)
    {
      LOG_WARN(logger) << "Cannot enumerate over directory: " << error.Message();
      continue;
    }

    // If we've already loaded a Lens by the same name, we ignore this one
    std::string name(g_file_info_get_name(info));

    if (std::find(lens_ids_.begin(), lens_ids_.end(), name) != lens_ids_.end())
    {
      g_debug ("Already in list");
      continue;
    }
    lens_ids_.push_back(name);

    g_debug("%s", g_file_info_get_name(info));
  }
}

Lenses::LensList FilesystemLenses::Impl::GetLenses() const
{
  LensList list;
  return list;
}

Lens::Ptr FilesystemLenses::Impl::GetLens(std::string const& lens_id) const
{
  Lens::Ptr p;
  return p;
}

Lens::Ptr FilesystemLenses::Impl::GetLensAtIndex(unsigned int index) const
{
  Lens::Ptr p;
  return p;
}

unsigned int FilesystemLenses::Impl::LensCount() const
{
  return 0;
}


FilesystemLenses::FilesystemLenses()
  : pimpl(new Impl(this))
{}

FilesystemLenses::FilesystemLenses(std::vector<std::string> const& lens_directories)
  :pimpl(new Impl(this, lens_directories))
{}

FilesystemLenses::~FilesystemLenses()
{
  delete pimpl;
}

Lenses::LensList FilesystemLenses::GetLenses() const
{
  return pimpl->GetLenses();
}

Lens::Ptr FilesystemLenses::GetLens(std::string const& lens_id) const
{
  return pimpl->GetLens(lens_id);
}

Lens::Ptr FilesystemLenses::GetLensAtIndex(unsigned int index) const
{
  return pimpl->GetLensAtIndex(index);
}

unsigned int FilesystemLenses::LensCount() const
{
  return pimpl->LensCount();
}


}
}
