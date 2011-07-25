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


namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.filesystemlenses");
}

static const char* GROUP = "Lens";

// Loads data from a Lens key-file in a usable form
struct LensFileData
{
  LensFileData(GKeyFile* file)
    : dbus_name(g_key_file_get_string(file, GROUP, "DBusName", NULL))
    , dbus_path(g_key_file_get_string(file, GROUP, "DBusPath", NULL))
    , name(g_key_file_get_locale_string(file, GROUP, "Name", NULL, NULL))
    , icon(g_key_file_get_string(file, GROUP, "Icon", NULL))
    , description(g_key_file_get_locale_string(file, GROUP, "Description", NULL, NULL))
    , search_hint(g_key_file_get_locale_string(file, GROUP, "SearchHint", NULL, NULL))
    , visible(g_key_file_get_boolean(file, GROUP, "Visible", NULL) ? true : false)
    , shortcut(g_key_file_get_string(file, GROUP, "Shortcut", NULL))
  {}

  static bool IsValid(GKeyFile* file, glib::Error& error)
  {
    return (g_key_file_has_group(file, GROUP) &&
            g_key_file_has_key(file, GROUP, "DBusName", error.AsOutParam()) &&
            g_key_file_has_key(file, GROUP, "DBusPath", error.AsOutParam()) &&
            g_key_file_has_key(file, GROUP, "Name", error.AsOutParam()) &&
            g_key_file_has_key(file, GROUP, "Icon", error.AsOutParam()));
  }

  glib::String dbus_name;
  glib::String dbus_path;
  glib::String name;
  glib::String icon;
  glib::String description;
  glib::String search_hint;
  bool         visible;
  glib::String shortcut;
};

/* A quick guide to finding Lens files
 *
 * We search one or multiple directories looking for folders with the following
 * layout (e.g. is for standard /usr/ installation):
 *
 * /usr/share/unity/lenses
 *                        /applications
 *                                     /applications.lens
 *                                     /applications.scope
 *                                     /chromium-webapps.scope
 *                        /files
 *                              /files.lens
 *                              /zeitgiest.scope
 *                              /ubuntuone.scope
 *
 * Etc, etc. We therefore need to enumerate these directories and find our
 * .lens files in them.
 *
 * Another note is that there is a priority system, where we want to let
 * .lens files found "the most local" to the user (say ~/.local/share)
 * override those found system-wide. This is to ease development of Lenses.
 *
 */
class FilesystemLenses::Impl
{
public:
  typedef std::map<GFile*, glib::Object<GCancellable>> CancellableMap;

  Impl(FilesystemLenses* owner);
  Impl(FilesystemLenses* owner, std::string const& lens_directory);

  ~Impl();

  List GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(unsigned int index) const;
  unsigned int count() const;

  void Init();
  glib::Object<GFile> BuildLensPathFileWithSuffix(std::string const& directory);
  static void OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl* self);
  void EnumerateLensesDirectoryChildren(GFileEnumerator* enumerator);
  void LoadLensFile(std::string const& lensfile_path);
  void CreateLensFromKeyFileData(GFile* path, const char* data, gsize length);

  FilesystemLenses* owner_;
  glib::Object<GFile> directory_;
  unsigned int children_waiting_to_load_;
  CancellableMap cancel_map_;
  List lenses_;
};

FilesystemLenses::Impl::Impl(FilesystemLenses* owner)
  : owner_(owner)
  , children_waiting_to_load_(0)
{
  LOG_DEBUG(logger) << "Initialising in standard lens directory mode";

  directory_ = BuildLensPathFileWithSuffix(DATADIR);

  Init();
}

FilesystemLenses::Impl::Impl(FilesystemLenses* owner, std::string const& lens_directory)
  : owner_(owner)
  , children_waiting_to_load_(0)
{
  LOG_DEBUG(logger) << "Initialising in override lens directory mode";

  directory_ = g_file_new_for_path(lens_directory.c_str());

  Init();
}

FilesystemLenses::Impl::~Impl()
{
for (std::pair<GFile*, glib::Object<GCancellable>> pair: cancel_map_)
  {
    g_cancellable_cancel(pair.second);
  }
}

void FilesystemLenses::Impl::Init()
{
  glib::String path(g_file_get_path(directory_));
  LOG_DEBUG(logger) << "Searching for Lenses in: " << path.Str();

  glib::Object<GCancellable> cancellable(g_cancellable_new());
  g_file_enumerate_children_async(directory_,
                                  G_FILE_ATTRIBUTE_STANDARD_NAME,
                                  G_FILE_QUERY_INFO_NONE,
                                  G_PRIORITY_DEFAULT,
                                  cancellable,
                                  (GAsyncReadyCallback)OnDirectoryEnumerated,
                                  this);
  cancel_map_[directory_] = cancellable;
}

glib::Object<GFile> FilesystemLenses::Impl::BuildLensPathFileWithSuffix(std::string const& directory)
{
  glib::String ret(g_build_filename(directory.c_str(), "unity", "lenses", NULL));
  glib::Object<GFile> file(g_file_new_for_path(ret.Value()));
  return file;
}

void FilesystemLenses::Impl::OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl* self)
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
    if (info && !error)
    {
      std::string name(g_file_info_get_name(info));
      glib::String dir_path(g_file_get_path(g_file_enumerator_get_container(enumerator)));
      auto lensfile_name = name + ".lens";

      glib::String lensfile_path(g_build_filename(dir_path.Value(),
                                                  name.c_str(),
                                                  lensfile_name.c_str(),
                                                  NULL));
      LoadLensFile(lensfile_path.Str());
    }
    else
    {
      LOG_WARN(logger) << "Cannot enumerate over directory: " << error.Message();
      continue;
    }
  }
}

void FilesystemLenses::Impl::LoadLensFile(std::string const& lensfile_path)
{
  glib::Object<GFile> file(g_file_new_for_path(lensfile_path.c_str()));

  // How many files are we waiting for to load
  children_waiting_to_load_++;

  auto loaded_cb = [](GObject * source, GAsyncResult * res, gpointer user_data)
  {
    Impl* self = static_cast<Impl*>(user_data);
    glib::Error error;
    char* contents = NULL;
    gsize length = 0;
    gboolean result;
    glib::String path(g_file_get_path(G_FILE(source)));

    result = g_file_load_contents_finish(G_FILE(source), res,
                                         &contents, &length,
                                         NULL, error.AsOutParam());
    if (result && !error)
    {
      self->CreateLensFromKeyFileData(G_FILE(source), contents, length);
      g_free(contents);
    }
    else
    {
      LOG_WARN(logger) << "Unable to read lens file "
      << path.Str() << ": "
      << error.Message();
    }

    // If we're not waiting for any more children to load, signal that we're
    // done reading the directory
    self->children_waiting_to_load_--;
    if (!self->children_waiting_to_load_)
      self->owner_->lenses_loaded.emit();
  };

  glib::Object<GCancellable> cancellable(g_cancellable_new());
  g_file_load_contents_async(file,
                             cancellable,
                             (GAsyncReadyCallback)(loaded_cb),
                             this);
  cancel_map_[file] = cancellable;
}

void FilesystemLenses::Impl::CreateLensFromKeyFileData(GFile* file,
                                                       const char* data,
                                                       gsize length)
{
  GKeyFile* key_file = g_key_file_new();
  glib::String path(g_file_get_path(file));
  glib::Error error;

  if (g_key_file_load_from_data(key_file, data, length, G_KEY_FILE_NONE, error.AsOutParam()))
  {
    if (LensFileData::IsValid(key_file, error))
    {
      LensFileData data(key_file);
      glib::String id(g_path_get_basename(path.Value()));

      Lens::Ptr lens(new Lens(id.Str(),
                              data.dbus_name.Str(),
                              data.dbus_path.Str(),
                              data.name.Str(),
                              data.icon.Str(),
                              data.description.Str(),
                              data.search_hint.Str(),
                              data.visible,
                              data.shortcut.Str()));
      lenses_.push_back(lens);
      owner_->lens_added.emit(lens);

      LOG_DEBUG(logger) << "Sucessfully loaded lens file " << path.Str();
    }
    else
    {
      LOG_WARN(logger) << "Lens file  "
                       << path.Str() << " is not valid: "
                       << error.Message();
    }
  }
  else
  {
    LOG_WARN(logger) << "Unable to load Lens file "
                     << path.Str() << ": "
                     << error.Message();
  }
  g_key_file_free(key_file);
}

Lenses::List FilesystemLenses::Impl::GetLenses() const
{
  return lenses_;
}

Lens::Ptr FilesystemLenses::Impl::GetLens(std::string const& lens_id) const
{
  Lens::Ptr p;

for (Lens::Ptr lens: lenses_)
  {
    std::string id = lens->id;
    if (id == lens_id)
      p = lens;
  }

  return p;
}

Lens::Ptr FilesystemLenses::Impl::GetLensAtIndex(unsigned int index) const
{
  return lenses_[index];
}

unsigned int FilesystemLenses::Impl::count() const
{
  return (unsigned int)lenses_.size();
}


FilesystemLenses::FilesystemLenses()
  : pimpl(new Impl(this))
{
  Init();
}

FilesystemLenses::FilesystemLenses(std::string const& lens_directory)
  : pimpl(new Impl(this, lens_directory))
{
  Init();
}

void FilesystemLenses::Init()
{
  count.SetGetterFunction(sigc::mem_fun(pimpl, &Impl::count));
}

FilesystemLenses::~FilesystemLenses()
{
  delete pimpl;
}

Lenses::List FilesystemLenses::GetLenses() const
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

}
}
