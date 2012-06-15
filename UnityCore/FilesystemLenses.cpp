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

#include <stdexcept>
#include <vector>

#include <gio/gio.h>
#include <glib.h>

#include <boost/lexical_cast.hpp>
#include <NuxCore/Logger.h>

#include "config.h"
#include "GLibWrapper.h"
#include "GLibSource.h"


namespace unity
{
namespace dash
{

namespace
{

nux::logging::Logger logger("unity.dash.filesystemlenses");
const char* GROUP = "Lens";

}

// Loads data from a Lens key-file in a usable form
LensDirectoryReader::LensFileData::LensFileData(GKeyFile* file,
                                                const gchar *lens_id)
  : id(g_strdup(lens_id))
  , domain(g_key_file_get_string(file, G_KEY_FILE_DESKTOP_GROUP, "X-Ubuntu-Gettext-Domain", NULL))
  , dbus_name(g_key_file_get_string(file, GROUP, "DBusName", NULL))
  , dbus_path(g_key_file_get_string(file, GROUP, "DBusPath", NULL))
  , name(g_strdup(g_dgettext(domain.Value(), glib::String(g_key_file_get_string(file, GROUP, "Name", NULL)))))
  , icon(g_key_file_get_string(file, GROUP, "Icon", NULL))
  , description(g_key_file_get_locale_string(file, GROUP, "Description", NULL, NULL))
  , search_hint(g_key_file_get_locale_string(file, GROUP, "SearchHint", NULL, NULL))
  , visible(true)
  , shortcut(g_key_file_get_string(file, GROUP, "Shortcut", NULL))
{
  if (g_key_file_has_key(file, GROUP, "Visible", NULL))
  {
    visible = g_key_file_get_boolean(file, GROUP, "Visible", NULL) != FALSE;
  }
}

bool LensDirectoryReader::LensFileData::IsValid(GKeyFile* file, glib::Error& error)
{
  return (g_key_file_has_group(file, GROUP) &&
          g_key_file_has_key(file, GROUP, "DBusName", &error) &&
          g_key_file_has_key(file, GROUP, "DBusPath", &error) &&
          g_key_file_has_key(file, GROUP, "Name", &error) &&
          g_key_file_has_key(file, GROUP, "Icon", &error));
}

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

class LensDirectoryReader::Impl
{
public:
  typedef std::map<GFile*, glib::Object<GCancellable>> CancellableMap;

  Impl(LensDirectoryReader *owner, std::string const& directory)
    : owner_(owner)
    , directory_(g_file_new_for_path(directory.c_str()))
    , children_waiting_to_load_(0)
    , enumeration_done_(false)
  {
    LOG_DEBUG(logger) << "Initialising lens reader for: " << directory;

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

  ~Impl()
  {
    for (auto pair: cancel_map_)
    {
      g_cancellable_cancel(pair.second);
    }
  }

  void EnumerateLensesDirectoryChildren(GFileEnumerator* enumerator);
  void LoadLensFile(std::string const& lensfile_path);
  void GetLensDataFromKeyFile(GFile* path, const char* data, gsize length);
  DataList GetLensData() const;
  void SortLensList();

  static void OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl* self);
  static void LoadFileContentCallback(GObject* source, GAsyncResult* res, gpointer user_data);

  LensDirectoryReader *owner_;
  glib::Object<GFile> directory_;
  DataList lenses_data_;
  std::size_t children_waiting_to_load_;
  bool enumeration_done_;
  CancellableMap cancel_map_;
};

void LensDirectoryReader::Impl::OnDirectoryEnumerated(GFile* source, GAsyncResult* res, Impl* self)
{
  glib::Error error;
  glib::Object<GFileEnumerator> enumerator(g_file_enumerate_children_finish(source, res, error.AsOutParam()));

  if (error || !enumerator)
  {
    glib::String path(g_file_get_path(source));
    LOG_WARN(logger) << "Unable to enumerate children of directory "
                     << path << " "
                     << error;
    return;
  }
  self->cancel_map_.erase(source);
  self->EnumerateLensesDirectoryChildren(enumerator);
}

void LensDirectoryReader::Impl::EnumerateLensesDirectoryChildren(GFileEnumerator* in_enumerator)
{
  glib::Object<GCancellable> cancellable(g_cancellable_new());

  cancel_map_[g_file_enumerator_get_container(in_enumerator)] = cancellable;
  g_file_enumerator_next_files_async (in_enumerator, 64, G_PRIORITY_DEFAULT,
                                      cancellable,
                                      [] (GObject *src, GAsyncResult *res,
                                          gpointer data) -> void {
    // async callback
    glib::Error error;
    GFileEnumerator *enumerator = G_FILE_ENUMERATOR (src);
    // FIXME: won't this kill the enumerator?
    GList *files = g_file_enumerator_next_files_finish (enumerator, res, error.AsOutParam());
    if (!error)
    {
      Impl *self = (Impl*) data;
      self->cancel_map_.erase(g_file_enumerator_get_container(enumerator));
      for (GList *iter = files; iter; iter = iter->next)
      {
        glib::Object<GFileInfo> info((GFileInfo*) iter->data);

        std::string name(g_file_info_get_name(info));
        glib::String dir_path(g_file_get_path(g_file_enumerator_get_container(enumerator)));
        std::string lensfile_name = name + ".lens";

        glib::String lensfile_path(g_build_filename(dir_path.Value(),
                                                    name.c_str(),
                                                    lensfile_name.c_str(),
                                                    NULL));
        self->LoadLensFile(lensfile_path.Str());
      }
      // the GFileInfos got already freed during the iteration
      g_list_free (files);
      self->enumeration_done_ = true;
    }
    else
    {
      LOG_WARN(logger) << "Cannot enumerate over directory: " << error;
    }
  }, this);
}

void LensDirectoryReader::Impl::LoadLensFile(std::string const& lensfile_path)
{
  glib::Object<GFile> file(g_file_new_for_path(lensfile_path.c_str()));
  glib::Object<GCancellable> cancellable(g_cancellable_new());

  // How many files are we waiting for to load
  children_waiting_to_load_++;

  g_file_load_contents_async(file,
                             cancellable,
                             (GAsyncReadyCallback)(LensDirectoryReader::Impl::LoadFileContentCallback),
                             this);
  cancel_map_[file] = cancellable;
}

void LensDirectoryReader::Impl::LoadFileContentCallback(GObject* source,
                                                        GAsyncResult* res,
                                                        gpointer user_data)
{
  Impl* self = static_cast<Impl*>(user_data);
  glib::Error error;
  glib::String contents;
  gsize length = 0;
  GFile* file = G_FILE(source);
  glib::String path(g_file_get_path(file));

  gboolean result = g_file_load_contents_finish(file, res,
                                                &contents, &length,
                                                NULL, error.AsOutParam());
  if (result)
  {
    self->GetLensDataFromKeyFile(file, contents.Value(), length);
    self->SortLensList();
  }
  else
  {
    LOG_WARN(logger) << "Unable to read lens file "
                     << path.Str() << ": "
                     << error;
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      return; // self is invalid now
  }

  self->cancel_map_.erase(file);

  // If we're not waiting for any more children to load, signal that we're
  // done reading the directory
  self->children_waiting_to_load_--;
  if (self->children_waiting_to_load_ == 0)
  {
    self->owner_->load_finished.emit();
  }
}

void LensDirectoryReader::Impl::GetLensDataFromKeyFile(GFile* file,
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
      glib::String id(g_path_get_basename(path.Value()));

      lenses_data_.push_back(LensFileDataPtr(new LensFileData(key_file, id)));

      LOG_DEBUG(logger) << "Sucessfully loaded lens file " << path;
    }
    else
    {
      LOG_WARN(logger) << "Lens file  "
                       << path << " is not valid: "
                       << error;
    }
  }
  else
  {
    LOG_WARN(logger) << "Unable to load Lens file "
                     << path << ": "
                     << error;
  }
  g_key_file_free(key_file);
}

LensDirectoryReader::DataList LensDirectoryReader::Impl::GetLensData() const
{
  return lenses_data_;
}

void LensDirectoryReader::Impl::SortLensList()
{
  //FIXME: We don't have a strict order, but alphabetical serves us well.
  // When we have an order/policy, please replace this.
  auto sort_cb = [] (LensFileDataPtr a, LensFileDataPtr b) -> bool
  {
    if (a->id.Str() == "applications.lens")
      return true;
    else if (b->id.Str() == "applications.lens")
      return false;
    else
      return g_strcmp0(a->id.Value(), b->id.Value()) < 0;
  };
  std::sort(lenses_data_.begin(),
            lenses_data_.end(),
            sort_cb);
}

LensDirectoryReader::LensDirectoryReader(std::string const& directory)
  : pimpl(new Impl(this, directory))
{
}

LensDirectoryReader::~LensDirectoryReader()
{
  delete pimpl;
}

LensDirectoryReader::Ptr LensDirectoryReader::GetDefault()
{
  static LensDirectoryReader::Ptr main_reader(new LensDirectoryReader(LENSES_DIR));

  return main_reader;
}

bool LensDirectoryReader::IsDataLoaded() const
{
  return pimpl->children_waiting_to_load_ == 0 && pimpl->enumeration_done_;
}

LensDirectoryReader::DataList LensDirectoryReader::GetLensData() const
{
  return pimpl->GetLensData();
}

class FilesystemLenses::Impl
{
public:
  Impl(FilesystemLenses* owner, LensDirectoryReader::Ptr const& reader);
  ~Impl()
  {
    finished_slot_.disconnect();
  }

  void OnLoadingFinished();

  LensList GetLenses() const;
  Lens::Ptr GetLens(std::string const& lens_id) const;
  Lens::Ptr GetLensAtIndex(std::size_t index) const;
  Lens::Ptr GetLensForShortcut(std::string const& lens_shortcut) const;
  std::size_t count() const;

  FilesystemLenses* owner_;
  LensDirectoryReader::Ptr reader_;
  LensList lenses_;
  sigc::connection finished_slot_;
  glib::Source::UniquePtr load_idle_;
};

FilesystemLenses::Impl::Impl(FilesystemLenses* owner, LensDirectoryReader::Ptr const& reader)
  : owner_(owner)
  , reader_(reader)
{
  finished_slot_ = reader_->load_finished.connect(sigc::mem_fun(this, &Impl::OnLoadingFinished));
  if (reader_->IsDataLoaded())
  {
    // we won't get any signal, so let's just emit our signals after construction
    load_idle_.reset(new glib::Idle([&] () {
      OnLoadingFinished();
      return false;
    }, glib::Source::Priority::DEFAULT));
  }
}

void FilesystemLenses::Impl::OnLoadingFinished()
{
  // FIXME: clear lenses_ first?
  for (auto lens_data : reader_->GetLensData())
  {
    Lens::Ptr lens(new Lens(lens_data->id,
                            lens_data->dbus_name,
                            lens_data->dbus_path,
                            lens_data->name,
                            lens_data->icon,
                            lens_data->description,
                            lens_data->search_hint,
                            lens_data->visible,
                            lens_data->shortcut));
    lenses_.push_back (lens);
  }

  for (Lens::Ptr& lens: lenses_)
    owner_->lens_added.emit(lens);

  owner_->lenses_loaded.emit();
}

Lenses::LensList FilesystemLenses::Impl::GetLenses() const
{
  return lenses_;
}

Lens::Ptr FilesystemLenses::Impl::GetLens(std::string const& lens_id) const
{
  for (auto lens: lenses_)
  {
    if (lens->id == lens_id)
    {
      return lens;
    }
  }

  return Lens::Ptr();
}

Lens::Ptr FilesystemLenses::Impl::GetLensAtIndex(std::size_t index) const
{
  try
  {
    return lenses_.at(index);
  }
  catch (std::out_of_range& error)
  {
    LOG_WARN(logger) << error.what();
  }
  return Lens::Ptr();
}

Lens::Ptr FilesystemLenses::Impl::GetLensForShortcut(std::string const& lens_shortcut) const
{
  for (auto lens: lenses_)
  {
    if (lens->shortcut == lens_shortcut)
    {
      return lens;
    }
  }

  return Lens::Ptr();
}

std::size_t FilesystemLenses::Impl::count() const
{
  return lenses_.size();
}


FilesystemLenses::FilesystemLenses()
  : pimpl(new Impl(this, LensDirectoryReader::GetDefault()))
{
  Init();
}

FilesystemLenses::FilesystemLenses(LensDirectoryReader::Ptr const& reader)
  : pimpl(new Impl(this, reader))
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

Lenses::LensList FilesystemLenses::GetLenses() const
{
  return pimpl->GetLenses();
}

Lens::Ptr FilesystemLenses::GetLens(std::string const& lens_id) const
{
  return pimpl->GetLens(lens_id);
}

Lens::Ptr FilesystemLenses::GetLensAtIndex(std::size_t index) const
{
  return pimpl->GetLensAtIndex(index);
}

Lens::Ptr FilesystemLenses::GetLensForShortcut(std::string const& lens_shortcut) const
{
  return pimpl->GetLensForShortcut(lens_shortcut);
}

}
}
