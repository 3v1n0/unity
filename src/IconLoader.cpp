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
  char      *key;

  g_return_if_fail (icon_name);
  g_return_if_fail (size > 1);

  key = Hash (icon_name, size);

  if (CacheLookup (key, icon_name, size, slot))
  {
    g_free (key);
    return;
  }
}

void
IconLoader::LoadFromGIconString (const char        *gicon_string,
                                 guint              size,
                                 IconLoaderCallback slot)
{

}

void
IconLoader::LoadFromFilename (const char        *filename,
                              guint              size,
                              IconLoaderCallback slot)
{

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
