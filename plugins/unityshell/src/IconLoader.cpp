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

#include <map>
#include <queue>
#include <sstream>

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
  // The Handle typedef is used to explicitly indicate which integers are
  // infact our opaque handles.
  typedef int Handle;

  Impl();
  ~Impl();

  Handle LoadFromIconName(std::string const& icon_name,
                          unsigned size,
                          IconLoaderCallback slot);

  Handle LoadFromGIconString(std::string const& gicon_string,
                             unsigned size,
                             IconLoaderCallback slot);

  Handle LoadFromFilename(std::string const& filename,
                          unsigned size,
                          IconLoaderCallback slot);

  Handle LoadFromURI(std::string const& uri,
                     unsigned size,
                     IconLoaderCallback slot);

  void DisconnectHandle(Handle handle);

private:

  enum IconLoaderRequestType
  {
    REQUEST_TYPE_ICON_NAME = 0,
    REQUEST_TYPE_GICON_STRING,
    REQUEST_TYPE_URI,
  };

  struct IconLoaderTask;
  typedef std::list<IconLoaderTask*> TaskList;

  struct IconLoaderTask
  {
    IconLoaderRequestType type;
    std::string           data;
    unsigned              size;
    std::string           key;
    IconLoaderCallback    slot;
    Handle                handle;
    Impl*                 self;
    GtkIconInfo*          icon_info;
    GdkPixbuf*            result;
    glib::Error           error;
    TaskList              shadow_tasks;

    IconLoaderTask(IconLoaderRequestType type_,
                   std::string const& data_,
                   unsigned size_,
                   std::string const& key_,
                   IconLoaderCallback slot_,
                   Handle handle_,
                   Impl* self_)
      : type(type_), data(data_), size(size_), key(key_)
      , slot(slot_), handle(handle_), self(self_)
      , icon_info(NULL), result(NULL)
      {}

    void InvokeSlot(GdkPixbuf* pixbuf)
    {
      slot(data, size, pixbuf);

      // notify shadow tasks
      for (auto shadow_task : shadow_tasks)
      {
        shadow_task->slot(shadow_task->data, shadow_task->size, pixbuf);

        self->task_map_.erase(shadow_task->handle);
        delete shadow_task;
      }

      shadow_tasks.clear();
    }
  };

  Handle ReturnCachedOrQueue(std::string const& data,
                             unsigned size,
                             IconLoaderCallback slot,
                             IconLoaderRequestType type);

  Handle QueueTask(std::string const& key,
                   std::string const& data,
                   unsigned size,
                   IconLoaderCallback slot,
                   IconLoaderRequestType type);

  std::string Hash(std::string const& data, unsigned size);

  bool CacheLookup(std::string const& key,
                   std::string const& data,
                   unsigned size,
                   IconLoaderCallback slot);

  // these methods might run asynchronously
  bool ProcessTask(IconLoaderTask* task);
  bool ProcessIconNameTask(IconLoaderTask* task);
  bool ProcessGIconTask(IconLoaderTask* task);
  bool ProcessURITask(IconLoaderTask* task);

  // Loading/rendering of pixbufs is done in a separate thread
  static gboolean LoaderJobFunc(GIOSchedulerJob* job, GCancellable *canc,
                                IconLoaderTask *task);
  static gboolean LoadIconComplete(IconLoaderTask* task);
  static gboolean CoalesceTasksCb(IconLoader::Impl* self);

  // Loop calls the iteration function.
  static gboolean Loop(Impl* self);
  bool Iteration();

private:
  typedef std::map<std::string, glib::Object<GdkPixbuf>> ImageCache;
  ImageCache cache_;
  typedef std::map<std::string, IconLoaderTask*> CacheQueue;
  CacheQueue queued_tasks_;
  typedef std::queue<IconLoaderTask*> TaskQueue;
  TaskQueue tasks_;
  typedef std::map<Handle, IconLoaderTask*> TaskMap;
  TaskMap task_map_;
  typedef std::vector<IconLoaderTask*> TaskArray;
  TaskArray finished_tasks_;

  guint idle_id_;
  guint coalesce_id_;
  bool no_load_;
  GtkIconTheme* theme_; // Not owned.
  Handle handle_counter_;
};


IconLoader::Impl::Impl()
  : idle_id_(0)
  , coalesce_id_(0)
  // Option to disable loading, if you're testing performance of other things
  , no_load_(::getenv("UNITY_ICON_LOADER_DISABLE"))
  , theme_(::gtk_icon_theme_get_default())
  , handle_counter_(0)
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


int IconLoader::Impl::LoadFromIconName(std::string const& icon_name,
                                       unsigned size,
                                       IconLoaderCallback slot)
{
  if (no_load_ || icon_name.empty() || size < MIN_ICON_SIZE)
    return 0;

  // We need to check this because of legacy desktop files
  if (icon_name[0] == '/')
  {
    return LoadFromFilename(icon_name, size, slot);
  }

  return ReturnCachedOrQueue(icon_name, size, slot, REQUEST_TYPE_ICON_NAME);
}

int IconLoader::Impl::LoadFromGIconString(std::string const& gicon_string,
                                          unsigned size,
                                          IconLoaderCallback slot)
{
  if (no_load_ || gicon_string.empty() || size < MIN_ICON_SIZE)
    return 0;

  return ReturnCachedOrQueue(gicon_string, size, slot, REQUEST_TYPE_GICON_STRING);
}

int IconLoader::Impl::LoadFromFilename(std::string const& filename,
                                       unsigned size,
                                       IconLoaderCallback slot)
{
  if (no_load_ || filename.empty() || size < MIN_ICON_SIZE)
    return 0;

  glib::Object<GFile> file(g_file_new_for_path(filename.c_str()));
  glib::String uri(g_file_get_uri(file));

  return LoadFromURI(uri.Str(), size, slot);
}

int IconLoader::Impl::LoadFromURI(std::string const& uri,
                                  unsigned size,
                                  IconLoaderCallback slot)
{
  if (no_load_ || uri.empty() || size < MIN_ICON_SIZE)
    return 0;

  return ReturnCachedOrQueue(uri, size, slot, REQUEST_TYPE_URI);
}

void IconLoader::Impl::DisconnectHandle(Handle handle)
{
  TaskMap::iterator iter = task_map_.find(handle);
  if (iter != task_map_.end())
  {
    iter->second->slot.disconnect();
  }
}

//
// Private Methods
//

int IconLoader::Impl::ReturnCachedOrQueue(std::string const& data,
                                          unsigned size,
                                          IconLoaderCallback slot,
                                          IconLoaderRequestType type)
{
  Handle result = 0;
  std::string key(Hash(data, size));

  if (!CacheLookup(key, data, size, slot))
  {
    result = QueueTask(key, data, size, slot, type);
  }
  return result;
}


int IconLoader::Impl::QueueTask(std::string const& key,
                                std::string const& data,
                                unsigned size,
                                IconLoaderCallback slot,
                                IconLoaderRequestType type)
{
  IconLoaderTask* task = new IconLoaderTask(type, data, size, key,
                                            slot, ++handle_counter_, this);

  auto iter = queued_tasks_.find(key);
  bool already_queued = iter != queued_tasks_.end();
  IconLoaderTask* running_task = already_queued ? iter->second : NULL;

  if (running_task != NULL)
  {
    running_task->shadow_tasks.push_back(task);
    // do NOT push the task into the tasks queue,
    // the parent task (which is in the queue) will handle it
    task_map_[task->handle] = task;

    LOG_DEBUG(logger) << "Appending shadow task  " << data
                      << ", queue size now at " << tasks_.size();

    return task->handle;
  }
  else
  {
    queued_tasks_[key] = task;
  }

  tasks_.push(task);
  task_map_[task->handle] = task;

  LOG_DEBUG(logger) << "Pushing task  " << data << " at size " << size
                    << ", queue size now at " << tasks_.size();

  if (idle_id_ == 0)
  {
    idle_id_ = g_idle_add_full(G_PRIORITY_LOW, (GSourceFunc)Loop, this, NULL);
  }
  return task->handle;
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

  LOG_DEBUG(logger) << "Processing  " << task->data << " at size " << task->size;

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
  task->InvokeSlot(nullptr);
  return true;
}

bool IconLoader::Impl::ProcessIconNameTask(IconLoaderTask* task)
{
  GtkIconInfo* info = gtk_icon_theme_lookup_icon(theme_,
                                                 task->data.c_str(),
                                                 task->size,
                                                 (GtkIconLookupFlags)0);
  if (info)
  {
    task->icon_info = info;
    g_io_scheduler_push_job ((GIOSchedulerJobFunc) LoaderJobFunc,
                             task, NULL, G_PRIORITY_HIGH_IDLE, NULL);

    return false;
  }
  else
  {
    LOG_WARNING(logger) << "Unable to load icon " << task->data
                        << " at size " << task->size;
  }

  task->InvokeSlot(nullptr);
  return true;
}

bool IconLoader::Impl::ProcessGIconTask(IconLoaderTask* task)
{
  glib::Error error;
  glib::Object<GIcon> icon(::g_icon_new_for_string(task->data.c_str(), &error));

  if (G_IS_FILE_ICON(icon.RawPtr()))
  {
    // [trasfer none]
    GFile* file = ::g_file_icon_get_file(G_FILE_ICON(icon.RawPtr()));
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
      task->icon_info = info;
      g_io_scheduler_push_job ((GIOSchedulerJobFunc) LoaderJobFunc,
                               task, NULL, G_PRIORITY_HIGH_IDLE, NULL);

      return false;
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

  task->InvokeSlot(nullptr);
  return true;
}

bool IconLoader::Impl::ProcessURITask(IconLoaderTask* task)
{
  g_io_scheduler_push_job ((GIOSchedulerJobFunc) LoaderJobFunc,
                           task, NULL, G_PRIORITY_HIGH_IDLE, NULL);

  return false;
}

gboolean IconLoader::Impl::LoaderJobFunc(GIOSchedulerJob* job,
                                         GCancellable *canc,
                                         IconLoaderTask *task)
{
  // careful here this is running in non-main thread
  if (task->icon_info)
  {
    task->result = gtk_icon_info_load_icon(task->icon_info, &task->error);

    gtk_icon_info_free (task->icon_info);
    task->icon_info = NULL;
  }
  else if (task->type == REQUEST_TYPE_URI)
  {
    glib::Object<GFile> file(g_file_new_for_uri(task->data.c_str()));
    glib::String contents;
    gsize length = 0;

    if (g_file_load_contents(file, canc, &contents, &length,
                             NULL, &task->error))
    {
      glib::Object<GInputStream> stream(
          g_memory_input_stream_new_from_data(contents.Value(), length, NULL));

      task->result = gdk_pixbuf_new_from_stream_at_scale(stream,
                                                         -1,
                                                         task->size,
                                                         TRUE,
                                                         canc,
                                                         &task->error);
      g_input_stream_close(stream, canc, NULL);
    }
  }

  g_io_scheduler_job_send_to_mainloop_async (job,
                                             (GSourceFunc) LoadIconComplete,
                                             task,
                                             NULL);

  return FALSE;
}

// this will be invoked back in the thread from which push_job was called
gboolean IconLoader::Impl::LoadIconComplete(IconLoaderTask* task)
{
  if (task->self->coalesce_id_ == 0)
  {
    // we're using lower priority than the GIOSchedulerJob uses to deliver
    // results to the mainloop
    task->self->coalesce_id_ =
      g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE + 10,
                          40,
                          (GSourceFunc) IconLoader::Impl::CoalesceTasksCb,
                          task->self,
                          NULL);
  }

  task->self->finished_tasks_.push_back (task);

  return FALSE;
}

gboolean IconLoader::Impl::CoalesceTasksCb(IconLoader::Impl* self)
{
  for (auto task : self->finished_tasks_)
  {
    // FIXME: we could update the cache sooner, but there are ref-counting
    // issues on the pixbuf (and inside the slot callbacks) that prevent us
    // from doing that.
    if (GDK_IS_PIXBUF(task->result))
    {
      task->self->cache_[task->key] = task->result;
    }
    else
    {
      LOG_WARNING(logger) << "Unable to load icon " << task->data
                          << " at size " << task->size << ": " << task->error;
    }

    task->InvokeSlot(task->result);

    // this was all async, we need to erase the task from the task_map
    self->task_map_.erase(task->handle);
    self->queued_tasks_.erase(task->key);
    delete task;
  }

  self->finished_tasks_.clear ();
  self->coalesce_id_ = 0;

  return FALSE;
}

bool IconLoader::Impl::Iteration()
{
  static const int MAX_MICRO_SECS = 1000;
  util::Timer timer;

  bool queue_empty = tasks_.empty();

  // always do at least one iteration if the queue isn't empty
  while (!queue_empty)
  {
    IconLoaderTask* task = tasks_.front();

    if (ProcessTask(task))
    {
      task_map_.erase(task->handle);
      queued_tasks_.erase(task->key);
      delete task;
    }

    tasks_.pop();
    queue_empty = tasks_.empty();

    if (timer.ElapsedMicroSeconds() >= MAX_MICRO_SECS) break;
  }

  LOG_DEBUG(logger) << "Iteration done, queue size now at " << tasks_.size();

  if (queue_empty)
  {
    idle_id_ = 0;
    if (task_map_.empty())
      handle_counter_ = 0;
  }

  return !queue_empty;
}


gboolean IconLoader::Impl::Loop(IconLoader::Impl* self)
{
  return self->Iteration() ? TRUE : FALSE;
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

int IconLoader::LoadFromIconName(std::string const& icon_name,
                                 unsigned size,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromIconName(icon_name, size, slot);
}

int IconLoader::LoadFromGIconString(std::string const& gicon_string,
                                    unsigned size,
                                    IconLoaderCallback slot)
{
  return pimpl->LoadFromGIconString(gicon_string, size, slot);
}

int IconLoader::LoadFromFilename(std::string const& filename,
                                 unsigned size,
                                 IconLoaderCallback slot)
{
  return pimpl->LoadFromFilename(filename, size, slot);
}

int IconLoader::LoadFromURI(std::string const& uri,
                            unsigned size,
                            IconLoaderCallback slot)
{
  return pimpl->LoadFromURI(uri, size, slot);
}

void IconLoader::DisconnectHandle(int handle)
{
  pimpl->DisconnectHandle(handle);
}


}
