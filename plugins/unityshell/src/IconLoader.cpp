/*
* Copyright (C) 2010 Canonical Ltd
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

#include <string.h>

#define _TEMPLATE_ "%s:%d"

IconLoader::IconLoader ()
: _idle_id (0)
{
  // Option to disable loading, if your testing performance of other things
  _no_load = g_getenv ("UNITY_ICON_LOADER_DISABLE") != NULL;

  _tasks = g_queue_new ();
  _theme = gtk_icon_theme_get_default ();
}

IconLoader::~IconLoader ()
{
  std::map<std::string, GdkPixbuf *>::iterator it;
  
  g_queue_free (_tasks);
  for (it=_cache.begin() ; it != _cache.end(); it++ )
    g_object_unref (GDK_PIXBUF ((*it).second));
}

IconLoader *
IconLoader::GetDefault ()
{
  static IconLoader *default_loader = NULL;

  if (G_UNLIKELY (!default_loader))
    default_loader = new IconLoader ();

  return default_loader;
}

void
IconLoader::LoadFromIconName (const char        *icon_name,
                              guint              size,
                              IconLoaderCallback slot)
{
  char *key;

  g_return_if_fail (icon_name);
  g_return_if_fail (size > 1);

  if (_no_load)
    return;

  // We need to check this because of legacy desktop files
  if (icon_name[0] == '/')
    LoadFromFilename (icon_name, size, slot);

  key = Hash (icon_name, size);

  if (CacheLookup (key, icon_name, size, slot))
  {
    g_free (key);
    return;
  }

  QueueTask (key, icon_name, size, slot, REQUEST_TYPE_ICON_NAME);

  g_free (key);
}

void
IconLoader::LoadFromGIconString (const char        *gicon_string,
                                 guint              size,
                                 IconLoaderCallback slot)
{
  char *key;

  g_return_if_fail (gicon_string);
  g_return_if_fail (size > 1);

  if (_no_load)
    return;

  key = Hash (gicon_string, size);

  if (CacheLookup (key, gicon_string, size, slot))
  {
    g_free (key);
    return;
  }

  QueueTask (key, gicon_string, size, slot, REQUEST_TYPE_GICON_STRING);

  g_free (key);
}

void
IconLoader::LoadFromFilename (const char        *filename,
                              guint              size,
                              IconLoaderCallback slot)
{
  GFile *file;
  gchar *uri;

  g_return_if_fail (filename);
  g_return_if_fail (size > 1);

  if (_no_load)
    return;

  file = g_file_new_for_path (filename);
  uri = g_file_get_uri (file);

  LoadFromURI (uri, size, slot);

  g_free (uri);
  g_object_unref (file);
}

void
IconLoader::LoadFromURI (const char        *uri,
                         guint              size,
                         IconLoaderCallback slot)
{
  char      *key;

  g_return_if_fail (uri);
  g_return_if_fail (size > 1);

  if (_no_load)
    return;

  key = Hash (uri, size);

  if (CacheLookup (key, uri, size, slot))
  {
    g_free (key);
    return;
  }

  QueueTask (key, uri, size, slot, REQUEST_TYPE_URI);

  g_free (key);
}

//
// Private Methods
//

void
IconLoader::QueueTask (const char           *key,
                       const char           *data,
                       guint                 size,
                       IconLoaderCallback    slot,
                       IconLoaderRequestType type)
{
  IconLoaderTask *task;

  if (g_strcmp0 (data, "") == 0)
  {
    slot (data, size, NULL);
    return;
  }

  task = g_slice_new0 (IconLoaderTask);
  task->key = g_strdup (key);
  task->data = g_strdup (data);
  task->size = size;
  task->slot = slot;
  task->type = type;
  task->self = this;

  g_queue_push_tail (_tasks, task);

  if (_idle_id < 1)
  {
    _idle_id = g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc)Loop, this, NULL);
    _idle_start_time = g_get_monotonic_time ();
  }
}

char *
IconLoader::Hash (const gchar *data, guint size)
{
  return g_strdup_printf (_TEMPLATE_, data, size);
}

bool
IconLoader::CacheLookup (const char *key,
                         const char *data,
                         guint       size,
                         IconLoaderCallback slot)
{
  GdkPixbuf *pixbuf;

  pixbuf = _cache[key];
  if (GDK_IS_PIXBUF (pixbuf))
  {
    slot (data, size, pixbuf);
    return true;
  }
  return false;
}

bool
IconLoader::ProcessTask (IconLoaderTask *task)
{
  GdkPixbuf *pixbuf = NULL;
  bool       task_complete = false;

  // Check the cache again, as previous tasks might have wanted the same
  if (CacheLookup (task->key, task->data, task->size, task->slot))
    return true;

  if (task->type == REQUEST_TYPE_ICON_NAME)
  {
    task_complete = ProcessIconNameTask (task);
  }
  else if (task->type == REQUEST_TYPE_GICON_STRING)
  {
    task_complete = ProcessGIconTask (task);
  }
  else if (task->type == REQUEST_TYPE_URI)
  {
    task_complete = ProcessURITask (task);
  }
  else
  {
    g_warning ("%s: Request type %d is not supported (%s %d)",
               G_STRFUNC,
               task->type,
               task->data,
               task->size);
    task->slot (task->data, task->size, pixbuf);
    task_complete = true;
  }

  return task_complete;
}

bool
IconLoader::ProcessIconNameTask (IconLoaderTask *task)
{
  GdkPixbuf   *pixbuf = NULL;
  GtkIconInfo *info = NULL;

  info = gtk_icon_theme_lookup_icon (_theme,
                                     task->data,
                                     task->size,
                                     (GtkIconLookupFlags)0);
  if (info != NULL)
  {
    GError *error = NULL;

    pixbuf = gtk_icon_info_load_icon (info, &error);
    if (GDK_IS_PIXBUF (pixbuf))
    {
      _cache[task->key] = pixbuf;
    }
    else
    {
      g_warning ("%s: Unable to load icon %s at size %d: %s",
                  G_STRFUNC,
                  task->data,
                  task->size,
                  error->message);
      g_error_free (error);
    }
    gtk_icon_info_free (info);
  }
  else
  {
    g_warning ("%s: Unable to load icon %s at size %d",
                G_STRFUNC,
                task->data,
                task->size);
  }

  task->slot (task->data, task->size, pixbuf);
  return true;
}

bool
IconLoader::ProcessGIconTask (IconLoaderTask *task)
{
  GdkPixbuf   *pixbuf = NULL;
  GIcon       *icon   = NULL;
  GError      *error  = NULL;

  if (!task)
    return false;

  icon = g_icon_new_for_string (task->data, &error);
  
  if (G_IS_FILE_ICON (icon))
  {
    GFile *file;
    bool   ret;

    // [trasfer none]
    file = g_file_icon_get_file (G_FILE_ICON (icon));

    g_free (task->data);
    task->type = REQUEST_TYPE_URI;
    task->data = g_file_get_uri (file);
    ret = ProcessURITask (task);

    g_object_unref (icon);

    return ret;
  }
  else if (G_IS_ICON (icon))
  {
    GtkIconInfo *info = NULL;
    info = gtk_icon_theme_lookup_by_gicon (_theme,
                                           icon,
                                           task->size,
                                           (GtkIconLookupFlags)0);
    if (info != NULL)
    {
      pixbuf = gtk_icon_info_load_icon (info, &error);

      if (GDK_IS_PIXBUF (pixbuf))
      {
        _cache[task->key] = pixbuf;
      }
      else
      {
        g_warning ("%s: Unable to load icon %s at size %d: %s",
                    G_STRFUNC,
                    task->data,
                    task->size,
                    error->message);
        g_error_free (error);
      }
      gtk_icon_info_free (info);
    }
    else
    {
      // There is some funkiness in some programs where they install
      // their icon to /usr/share/icons/hicolor/apps/, but they
      // name the Icon= key as `foo.$extension` which breaks loading
      // So we can try and work around that here.
      if (g_str_has_suffix (task->data, ".png")
          || g_str_has_suffix (task->data, ".xpm")
          || g_str_has_suffix (task->data, ".gif")
          || g_str_has_suffix (task->data, ".jpg"))
      {
        char *new_data;
        
        new_data = g_strndup (task->data, strlen (task->data) - 4);
        g_free (task->data);
        task->data = new_data;
        return ProcessIconNameTask (task);
      }
      else
      {
        g_warning ("%s: Unable to load icon %s at size %d",
                    G_STRFUNC,
                    task->data,
                    task->size);
      }
    }
    g_object_unref (icon);
  }
  else
  {
    g_warning ("%s: Unable to load GIcon %s at size %d: %s",
                G_STRFUNC,
                task->data,
                task->size,
                error->message);
    g_error_free (error);
  }

  task->slot (task->data, task->size, pixbuf);
  return true;
}

bool
IconLoader::ProcessURITask (IconLoaderTask *task)
{
  GFile *file;

  file = g_file_new_for_uri (task->data);

  g_file_load_contents_async (file,
                              NULL,
                              (GAsyncReadyCallback)LoadContentsReady,
                              task);
  g_object_unref (file);

  return false;
}

void
IconLoader::ProcessURITaskReady (IconLoaderTask *task, char *contents, gsize length)
{
  GdkPixbuf    *pixbuf = NULL;
  GInputStream *stream;
  GError       *error = NULL;

  stream = g_memory_input_stream_new_from_data (contents, length, NULL);
  pixbuf = gdk_pixbuf_new_from_stream_at_scale (stream,
                                                -1,
                                                task->size,
                                                true,
                                                NULL,
                                                &error);
  if (GDK_IS_PIXBUF (pixbuf))
  {
    _cache[task->key] = pixbuf;
  }
  else
  {
    g_warning ("%s: Unable to create pixbuf from input stream for %s: %s",
               G_STRFUNC,
               task->data,
               error->message);
    g_error_free (error);
  }

  task->slot (task->data, task->size, pixbuf);

  FreeTask (task);
  g_input_stream_close (stream, NULL, NULL);
}

bool
IconLoader::Iteration ()
{
#define MAX_MICRO_SECS 10000
  bool is_empty;
  gint64 time;

  time = g_get_monotonic_time ();
  
  while (true)
  {
    IconLoaderTask *task;

    task = static_cast<IconLoaderTask *> (g_queue_pop_head (_tasks));
    if (!task)
      break;

    if (ProcessTask (task))
      FreeTask (task);

    if (g_get_monotonic_time () - time > MAX_MICRO_SECS)
      break;
  }

  is_empty = g_queue_is_empty (_tasks);
  if (is_empty)
  {
    _idle_id = 0;
  }

  return !is_empty;
}

void
IconLoader::FreeTask (IconLoaderTask *task)
{
  g_free (task->key);
  g_free (task->data);
  g_slice_free (IconLoaderTask, task);
}

bool
IconLoader::Loop (IconLoader *self)
{
  return static_cast<IconLoader *> (self)->Iteration ();
}

void
IconLoader::LoadContentsReady (GObject *obj, GAsyncResult *res, IconLoaderTask *task)
{
  char   *contents = NULL;
  gsize   length = 0;
  GError *error = NULL;

  if (g_file_load_contents_finish (G_FILE (obj), res, &contents, &length, NULL, &error))
  {
    task->self->ProcessURITaskReady (task, contents, length);

    g_free (contents);
  }
  else
  {
    g_warning ("%s: Unable to load contents of %s: %s",
               G_STRFUNC,
               task->data,
               error->message);
    g_error_free (error);
  }
}
