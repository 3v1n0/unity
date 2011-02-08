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

#ifndef ICON_LOADER_H
#define ICON_LOADER_H

#include <string>
#include <map>
#include <vector>
#include <sigc++/sigc++.h>
#include <gtk/gtk.h>

class IconLoader
{
public:
  typedef sigc::slot<void, const char *, guint, GdkPixbuf *> IconLoaderCallback;

  IconLoader ();
  ~IconLoader ();

 // DO NOT `delete` this...ever.
 static IconLoader * GetDefault ();

 void LoadFromIconName    (const char        *icon_name,
                           guint              size,
                           IconLoaderCallback slot);

 void LoadFromGIconString (const char        *gicon_string,
                           guint              size,
                           IconLoaderCallback slot);

 void LoadFromFilename    (const char        *filename,
                           guint              size,
                           IconLoaderCallback slot);
private:

  enum IconLoaderRequestType
  {
    ICON_NAME=0,
    ICON_STRING
  };

  class IconLoaderTask
  {
  public:
    IconLoaderTask (IconLoaderRequestType type,
                    char                 *data,
                    guint                 size,
                    char                 *key,
                    sigc::slot<void, char *, guint, GdkPixbuf *> slot)
    : _type (type),
      _data (data),
      _size (size),
      _key (key),
      _slot (slot)
    {

    }

    ~IconLoaderTask ()
    {
      g_free (_data);
      g_free (_key);
    }

    IconLoaderRequestType  _type;
    char                  *_data;
    guint                  _size;
    char                  *_key;
    sigc::slot<void, char*, guint, GdkPixbuf *> _slot;
  };

  char * Hash (const gchar *data, guint size);
  bool   CacheLookup (const char *key,
                      const char *data,
                      guint       size,
                      IconLoaderCallback slot);
 
private:
 std::map<std::string, GdkPixbuf *> _cache;
 std::vector<IconLoaderTask>        _tasks;
};

#endif // ICON_LOADER_H
