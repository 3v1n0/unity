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

#define _TEMPLATE_ "%s:%d"

IconLoader::IconLoader ()
{

}

IconLoader::~IconLoader ()
{

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
  char      *key;

  g_return_if_fail (filename);
  g_return_if_fail (size > 1);

  key = Hash (filename, size);

  if (CacheLookup (key, filename, size, slot))
  {
    g_free (key);
    return;
  }

  QueueTask (key, filename, size, slot, REQUEST_TYPE_FILENAME);

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

  task = g_slice_new0 (IconLoaderTask);
  task->key = g_strdup (key);
  task->data = g_strdup (data);
  task->size = size;
  task->slot = slot;
  task->type = type;

  _tasks.push_back (task);
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
