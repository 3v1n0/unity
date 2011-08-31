// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2010, 2011 Canonical Ltd
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

#include "IconLoader.h"

#include <queue>

#include <boost/algorithm/string.hpp>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <string.h>

#include "Timer.h"

namespace unity
{
namespace
{
nux::logging::Logger logger("unity.iconloader");
const unsigned MIN_ICON_SIZE = 2;
}

class IconLoader::Impl
{
public:
  Impl();
  ~Impl();

  void LoadFromIconName(std::string const& icon_name,
                        unsigned size,
                        IconLoaderCallback slot);

  void LoadFromGIconString(std::string const& gicon_string,
                           unsigned size,
                           IconLoaderCallback slot);

  void LoadFromFilename(std::string const& filename,
                        unsigned size,
                        IconLoaderCallback slot);

  void LoadFromURI(std::string const& uri,
                   unsigned size,
                   IconLoaderCallback slot);

private:

  enum IconLoaderRequestType
  {
    REQUEST_TYPE_ICON_NAME = 0,
    REQUEST_TYPE_GICON_STRING,
    REQUEST_TYPE_URI,
  };

  struct IconLoaderTask
  {
    IconLoaderRequestType type;
    std::string           data;
    unsigned              size;
    std::string           key;
    IconLoaderCallback    slot;
    Impl*                 self;

    IconLoaderTask(IconLoaderRequestType type_,
                   std::string const& data_,
                   unsigned size_,
                   std::string const& key_,
                   IconLoaderCallback slot_,
                   Impl* self_)
      : type(type_), data(data_), size(size_), key(key_), slot(slot_), self(self_)
      {}
  };

  void ReturnCachedOrQueue(std::string const& data,
                           unsigned size,
                           IconLoaderCallback slot,
                           IconLoaderRequestType type);

  void QueueTask(std::string const& key,
                 std::string const& data,
                 unsigned size,
                 IconLoaderCallback slot,
                 IconLoaderRequestType type);

  std::string Hash(std::string const& data, unsigned size);

  bool CacheLookup(std::string const& key,
                   std::string const& data,
                   unsigned size,
                   IconLoaderCallback slot);

  bool ProcessTask(IconLoaderTask* task);
  bool ProcessIconNameTask(IconLoaderTask* task);
  bool ProcessGIconTask(IconLoaderTask* task);

  // URI processing is async.
  bool ProcessURITask(IconLoaderTask* task);
  void ProcessURITaskReady(IconLoaderTask* task, char* contents, gsize length);
  static void LoadContentsReady(GObject* object, GAsyncResult* res, IconLoaderTask* task);

  // Loop calls the iteration function.
  static bool Loop(Impl* self);
  bool Iteration();

private:
  typedef std::map<std::string, glib::Object<GdkPixbuf>> ImageCache;
  ImageCache cache_;
  typedef std::queue<IconLoaderTask*> TaskQueue;
  TaskQueue tasks_;

  guint idle_id_;
  bool no_load_;
  GtkIconTheme* theme_; // Not owned.
};


IconLoader::Impl::Impl()
  : idle_id_(0)
  // Option to disable loading, if your testing performance of other things
  , no_load_(::getenv("UNITY_ICON_LOADER_DISABLE"))
  , theme_(::gtk_icon_theme_get_default())
{
}

IconLoader::Impl::~Impl()
{
  while (!tasks_.empty())
  {
    delete tasks_.front();
    tasks_.pop();
  }
}


void IconLoader::Impl::LoadFromIconName(std::string const& icon_name,
                                        unsigned size,
                                        IconLoaderCallback slot)
{
  if (no_load_ || icon_name.empty() || size < MIN_ICON_SIZE)
    return;

  // We need to check this because of legacy desktop files
  if (icon_name[0] == '/')
  {
    LoadFromFilename(icon_name, size, slot);
    return;
  }

  ReturnCachedOrQueue(icon_name, size, slot, REQUEST_TYPE_ICON_NAME);
}

void IconLoader::Impl::LoadFromGIconString(std::string const& gicon_string,
                                           unsigned size,
                                           IconLoaderCallback slot)
{
  if (no_load_ || gicon_string.empty() || size < MIN_ICON_SIZE)
    return;

  ReturnCachedOrQueue(gicon_string, size, slot, REQUEST_TYPE_GICON_STRING);
}

void IconLoader::Impl::LoadFromFilename(std::string const& filename,
                                        unsigned size,
                                        IconLoaderCallback slot)
{
  if (no_load_ || filename.empty() || size < MIN_ICON_SIZE)
    return;

  glib::Object<GFile> file(g_file_new_for_path(filename.c_str()));
  glib::String uri(g_file_get_uri(file));

  LoadFromURI(uri.Str(), size, slot);
}

void IconLoader::Impl::LoadFromURI(std::string const& uri,
                                   unsigned size,
                                   IconLoaderCallback slot)
{
  if (no_load_ || uri.empty() || size < MIN_ICON_SIZE)
    return;

  ReturnCachedOrQueue(uri, size, slot, REQUEST_TYPE_URI);
}

//
// Private Methods
//

void IconLoader::Impl::ReturnCachedOrQueue(std::string const& data,
                                           unsigned size,
                                           IconLoaderCallback slot,
                                           IconLoaderRequestType type)
{
  std::string key(Hash(data, size));

  if (!CacheLookup(key, data, size, slot))
  {
    QueueTask(key, data, size, slot, type);
  }
}


void IconLoader::Impl::QueueTask(std::string const& key,
                                 std::string const& data,
                                 unsigned size,
                                 IconLoaderCallback slot,
                                 IconLoaderRequestType type)
{
  tasks_.push(new IconLoaderTask(type, data, size, key, slot, this));

  if (idle_id_ == 0)
  {
    idle_id_ = g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)Loop, this, NULL);
  }
}

std::string IconLoader::Impl::Hash(std::string const& data, unsigned size)
{
  std::ostringstream sout;
  sout << data << ":" << size;
  return sout.str();
}

bool IconLoader::Impl::CacheLookup(std::string const& key,
                                   std::string const& data,
                                   unsigned size,
                                   IconLoaderCallback slot)
{
  auto iter = cache_.find(key);
  bool found = iter != cache_.end();
  if (found)
  {
    GdkPixbuf* pixbuf = iter->second;
    slot(data, size, pixbuf);
  }
  return found;
}

bool IconLoader::Impl::ProcessTask(IconLoaderTask* task)
{
  // Check the cache again, as previous tasks might have wanted the same
  if (CacheLookup(task->key, task->data, task->size, task->slot))
    return true;

  // Rely on the compiler to tell us if we miss a new type
  switch (task->type)
  {
  case REQUEST_TYPE_ICON_NAME:
    return ProcessIconNameTask(task);
  case REQUEST_TYPE_GICON_STRING:
    return ProcessGIconTask(task);
  case REQUEST_TYPE_URI:
    return ProcessURITask(task);
  }

  LOG_WARNING(logger) << "Request type " << task->type
                      << " is not supported (" << task->data
                      << " " << task->size << ")";
  task->slot(task->data, task->size, nullptr);
  retrun true;
}

bool IconLoader::Impl::ProcessIconNameTask(IconLoaderTask* task)
{
  GdkPixbuf* pixbuf = nullptr;
  GtkIconInfo* info = gtk_icon_theme_lookup_icon(theme_,
                                                 task->data,
                                                 task->size,
                                                 (GtkIconLookupFlags)0);
  if (info)
  {
    glib::Error error;

    pixbuf = gtk_icon_info_load_icon(info, &error);
    if (GDK_IS_PIXBUF(pixbuf))
    {
      cache_[task->key] = pixbuf;
    }
    else
    {
      LOG_WARNING(logger) << "Unable to load icon " << task->data
                          << " at size " << task->size << ": " << error;
    }
    gtk_icon_info_free(info);
  }
  else
  {
    LOG_WARNING(logger) << "Unable to load icon " << task->data
                        << " at size " << task->size;
  }

  task->slot(task->data, task->size, pixbuf);
  return true;
}

bool IconLoader::Impl::ProcessGIconTask(IconLoaderTask* task)
{
  GdkPixbuf*   pixbuf = NULL;

  glib::Error error;
  glib::Object<GIcon> icon(::g_icon_new_for_string(task->data, &error));

  if (G_IS_FILE_ICON(icon.RawPtr()))
  {
    // [trasfer none]
    GFile* file = ::g_file_icon_get_file(G_FILE_ICON(icon));
    glib::String uri(::g_file_get_uri(file));

    task->type = REQUEST_TYPE_URI;
    task->data = uri.Str();
    return ProcessURITask(task);
  }
  else if (G_IS_ICON(icon.RawPtr()))
  {
    GtkIconInfo* info = ::gtk_icon_theme_lookup_by_gicon(theme_,
                                                         icon,
                                                         task->size,
                                                         (GtkIconLookupFlags)0);
    if (info)
    {
      pixbuf = gtk_icon_info_load_icon(info, &error);

      if (GDK_IS_PIXBUF(pixbuf))
      {
        cache_[task->key] = pixbuf;
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load icon " << task->data
                            << " at size " << task->size << ": " << error;
      }
      gtk_icon_info_free(info);
    }
    else
    {
      // There is some funkiness in some programs where they install
      // their icon to /usr/share/icons/hicolor/apps/, but they
      // name the Icon= key as `foo.$extension` which breaks loading
      // So we can try and work around that here.
      std::string& data = task->data;
      if (boost::iends_with(data, ".png") ||
          boost::iends_with(data, ".xpm") ||
          boost::iends_with(data, ".gif") ||
          boost::iends_with(data, ".jpg"))
      {
        data = data.substr(0, data.size() - 4);
        return ProcessIconNameTask(task);
      }
      else
      {
        LOG_WARNING(logger) << "Unable to load icon " << task->data
                            << " at size " << task->size;
      }
    }
  }
  else
  {
    LOG_WARNING(logger) << "Unable to load icon " << task->data
                        << " at size " << task->size << ": " << error;
  }

  task->slot(task->data, task->size, pixbuf);
  return true;
}

bool IconLoader::Impl::ProcessURITask(IconLoaderTask* task)
{
  glib::Object<GFile> file(g_file_new_for_uri(task->data));

  g_file_load_contents_async(file,
                             NULL,
                             (GAsyncReadyCallback)LoadContentsReady,
                             task);

  return false;
}

void IconLoader::Impl::ProcessURITaskReady(IconLoaderTask* task,
                                           char* contents,
                                           gsize length)
{
  GInputStream* stream = g_memory_input_stream_new_from_data(contents, length, NULL);

  glib::Error error;
  glib::Object<GdkPixbuf> pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream,
                                                                       -1,
                                                                       task->size,
                                                                       true,
                                                                       NULL,
                                                                       &error);
  if (error)
  {
    LOG_WARNING(logger) << "Unable to create pixbuf from input stream for "
                        << task->data << " at size " << task->size << ": " << error;
  }
  else
  {
    cache_[task->key] = pixbuf;
  }

  task->slot(task->data, task->size, pixbuf);
  g_input_stream_close(stream, NULL, NULL);
}

bool IconLoader::Impl::Iteration()
{
  static const int MAX_MICRO_SECS = 10000;
  util::Timer timer;

  bool queue_empty = tasks_.empty();

  while (!queue_empty &&
         (timer.ElapsedMicroSeconds() < MAX_MICRO_SECS))
  {
    IconLoaderTask* task = tasks_.front();

    if (ProcessTask(task))
    {
      delete task;
    }

    tasks_.pop();
    queue_empty = tasks_.empty();
  }

  if (queue_empty)
  {
    idle_id_ = 0;
  }

  return !queue_empty;
}


bool IconLoader::Impl::Loop(IconLoader::Impl* self)
{
  return self->Iteration();
}

void IconLoader::Impl::LoadContentsReady(GObject* obj,
                                         GAsyncResult* res,
                                         IconLoaderTask* task)
{
  glib::Sring contents;
  glib::Error error;
  gsize length = 0;

  if (g_file_load_contents_finish(G_FILE(obj), res, &contents, &length, NULL, &error))
  {
    task->self->ProcessURITaskReady(task, contents.Value(), length);
  }
  else
  {
    LOG_WARNING(logger) << "Unable to load contents of "
                        << task->data << ": " << error;
  }
  delete task;
}


IconLoader::IconLoader()
  : pimpl(new Impl())
{
}

IconLoader::~IconLoader()
{
  delete pimpl;
}

IconLoader& IconLoader::GetDefault()
{
  static IconLoader default_loader;
  return default_loader;
}

void IconLoader::LoadFromIconName(const char* icon_name,
                                  guint size,
                                  IconLoaderCallback slot)
{
  pimpl->LoadFromIconName(icon_name, size, slot);
}

void IconLoader::LoadFromGIconString(const char* gicon_string,
                                     guint size,
                                     IconLoaderCallback slot)
{
  pimpl->LoadFromGIconString(gicon_string, size, slot);
}

void IconLoader::LoadFromFilename(const char* filename,
                                  guint size,
                                  IconLoaderCallback slot)
{
  pimpl->LoadFromFilename(filename, size, slot);
}

void IconLoader::LoadFromURI(const char* uri,
                             guint size,
                             IconLoaderCallback slot)
{
  pimpl->LoadFromURI(uri, size, slot);
}


}
