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
  typedef sigc::slot<void, const char*, guint, GdkPixbuf*> IconLoaderCallback;

  IconLoader();
  ~IconLoader();

// DO NOT `delete` this...ever.
  static IconLoader* GetDefault();

  int LoadFromIconName(const char*        icon_name,
                       guint              size,
                       IconLoaderCallback slot);

  int LoadFromGIconString(const char*        gicon_string,
                          guint              size,
                          IconLoaderCallback slot);

  int LoadFromFilename(const char*        filename,
                       guint              size,
                      IconLoaderCallback slot);

  int LoadFromURI(const char*        uri,
                  guint              size,
                  IconLoaderCallback slot);

  void DisconnectHandle (int handle);
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
    char*                 data;
    guint                 size;
    char*                 key;
    IconLoaderCallback    slot;
    IconLoader*           self;
    int                   handle;
  };

  static int TaskCompareHandle (IconLoaderTask* task, int* b);

  int    QueueTask(const char*           key,
                   const char*           data,
                   guint                 size,
                   IconLoaderCallback    slot,
                   IconLoaderRequestType type);
  char* Hash(const char* data, guint size);
  bool   CacheLookup(const char* key,
                     const char* data,
                     guint       size,
                     IconLoaderCallback slot);
  bool   ProcessTask(IconLoaderTask* task);
  bool   ProcessIconNameTask(IconLoaderTask* task);
  bool   ProcessGIconTask(IconLoaderTask* task);
  bool   ProcessURITask(IconLoaderTask* task);
  void   ProcessURITaskReady(IconLoaderTask* task, char* contents, gsize length);
  bool   Iteration();
  void   FreeTask(IconLoaderTask* task);

  static bool Loop(IconLoader* self);
  static void LoadContentsReady(GObject* object, GAsyncResult* res, IconLoaderTask* task);

  int id_sequencial_number_;

private:
  bool _no_load;

  std::map<std::string, GdkPixbuf*> _cache;
  GQueue*       _tasks;
  guint         _idle_id;
  GtkIconTheme* _theme;
  gint64        _idle_start_time;
};

#endif // ICON_LOADER_H
