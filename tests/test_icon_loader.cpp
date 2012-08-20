/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Michal Hruby <michal.hruby@canonical.com>
 */

#include <gmock/gmock.h>
#include <sigc++/sigc++.h>

#include "IconLoader.h"

using namespace testing;
using namespace unity;

namespace
{
bool IsValidPixbuf(GdkPixbuf *pixbuf)
{
  return GDK_IS_PIXBUF (pixbuf);
}

gboolean TimeoutReached (gpointer data)
{
  bool *b = static_cast<bool*>(data);

  *b = true;

  return FALSE;
}

struct LoadResult
{
  glib::Object<GdkPixbuf> pixbuf;
  bool got_callback;

  LoadResult() : pixbuf(NULL), got_callback(false) {}
  void IconLoaded(std::string const& icon_name, unsigned size,
                  glib::Object<GdkPixbuf> const& buf)
  {
    pixbuf = buf;

    got_callback = true;
  }
};

TEST(TestIconLoader, TestGetDefault)
{
  // we need to initialize gtk
  int args_cnt = 0;
  gtk_init (&args_cnt, NULL);

  IconLoader::GetDefault();
}

TEST(TestIconLoader, TestGetOneIcon)
{
  LoadResult load_result;
  IconLoader& icon_loader = IconLoader::GetDefault();
  volatile bool timeout_reached = false;

  icon_loader.LoadFromIconName("gedit-icon", 48, sigc::mem_fun(load_result,
        &LoadResult::IconLoaded));

  guint tid = g_timeout_add (10000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !load_result.got_callback)
  {
    g_main_context_iteration (NULL, TRUE);
  }

  EXPECT_TRUE(load_result.got_callback);
  EXPECT_TRUE(IsValidPixbuf(load_result.pixbuf));

  g_source_remove (tid);
}

TEST(TestIconLoader, TestGetAnnotatedIcon)
{
  LoadResult load_result;
  IconLoader& icon_loader = IconLoader::GetDefault();
  volatile bool timeout_reached = false;

  
  icon_loader.LoadFromGIconString(". UnityProtocolAnnotatedIcon %7B'base-icon':%20%3C'gedit-icon'%3E,%20'ribbon':%20%3C'foo'%3E%7D", 48, sigc::mem_fun(load_result,
        &LoadResult::IconLoaded));

  guint tid = g_timeout_add (10000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !load_result.got_callback)
  {
    g_main_context_iteration (NULL, TRUE);
  }

  EXPECT_TRUE(load_result.got_callback);
  EXPECT_TRUE(IsValidPixbuf(load_result.pixbuf));

  g_source_remove (tid);
}

TEST(TestIconLoader, TestGetOneIconManyTimes)
{
  std::vector<LoadResult> results;
  std::vector<int> handles;
  IconLoader& icon_loader = IconLoader::GetDefault();
  volatile bool timeout_reached = false;
  int i, load_count;

  // 100 times should be good
  load_count = 100;
  results.resize (load_count);
  handles.resize (load_count);

  // careful, don't use the same icon as in previous tests, otherwise it'll
  // be cached already!
  for (int i = 0; i < load_count; i++)
  {
    handles[i] = icon_loader.LoadFromIconName("web-browser", 48,
        sigc::mem_fun(results[i], &LoadResult::IconLoaded));
  }

  // disconnect every other handler (and especially the first one)
  for (i = 0; i < load_count; i += 2)
  {
    icon_loader.DisconnectHandle(handles[i]);
  }

  guint tid = g_timeout_add (10000, TimeoutReached, (gpointer)(&timeout_reached));
  int iterations = 0;
  while (!timeout_reached)
  {
    g_main_context_iteration (NULL, TRUE);
    bool all_loaded = true;
    bool any_loaded = false;
    for (i = 1; i < load_count; i += 2)
    {
      all_loaded &= results[i].got_callback;
      any_loaded |= results[i].got_callback;
      if (!all_loaded) break;
    }
    // count the number of iterations where we got some results
    if (any_loaded) iterations++;
    if (all_loaded) break;
  }

  EXPECT_FALSE(timeout_reached);
  // it's all loading the same icon, the results had to come in the same
  // main loop iteration (that's the desired behaviour)
  EXPECT_EQ(iterations, 1);

  for (i = 0; i < load_count; i++)
  {
    if (i % 2)
    {
      EXPECT_TRUE(results[i].got_callback);
      EXPECT_TRUE(IsValidPixbuf(results[i].pixbuf));
    }
    else
    {
      EXPECT_FALSE(results[i].got_callback);
    }
  }

  g_source_remove (tid);
}

TEST(TestIconLoader, TestGetManyIcons)
{
  std::vector<LoadResult> results;
  IconLoader& icon_loader = IconLoader::GetDefault();
  volatile bool timeout_reached = false;
  int i = 0;
  int icon_count;

  GList *icons = gtk_icon_theme_list_icons (gtk_icon_theme_get_default (),
                                            "Applications");
  // loading 100 icons should suffice
  icon_count = MIN (100, g_list_length (icons));
  results.resize (icon_count);
  for (GList *it = icons; it != NULL; it = it->next)
  {
    const char *icon_name = static_cast<char*>(it->data);
    icon_loader.LoadFromIconName(icon_name, 48, sigc::mem_fun(results[i++],
        &LoadResult::IconLoaded));
    if (i >= icon_count) break;
  }

  guint tid = g_timeout_add (30000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached)
  {
    g_main_context_iteration (NULL, TRUE);
    bool all_loaded = true;
    for (auto loader: results)
    {
      all_loaded &= loader.got_callback;
      if (!all_loaded) break;
    }
    if (all_loaded) break;
  }

  for (auto load_result: results)
  {
    EXPECT_TRUE(load_result.got_callback);
    EXPECT_TRUE(IsValidPixbuf(load_result.pixbuf));
  }

  g_source_remove (tid);
}

TEST(TestIconLoader, TestCancelSome)
{
  std::vector<LoadResult> results;
  std::vector<int> handles;
  IconLoader& icon_loader = IconLoader::GetDefault();
  volatile bool timeout_reached = false;
  int i = 0;
  int icon_count;

  GList *icons = gtk_icon_theme_list_icons (gtk_icon_theme_get_default (),
                                            "Emblems");
  // loading 100 icons should suffice
  icon_count = MIN (100, g_list_length (icons));
  results.resize (icon_count);
  handles.resize (icon_count);
  for (GList *it = icons; it != NULL; it = it->next)
  {
    const char *icon_name = static_cast<char*>(it->data);
    int handle = icon_loader.LoadFromIconName(icon_name, 48, sigc::mem_fun(
          results[i], &LoadResult::IconLoaded));
    handles[i++] = handle;
    if (i >= icon_count) break;
  }

  // disconnect every other handler
  for (i = 0; i < icon_count; i += 2)
  {
    icon_loader.DisconnectHandle(handles[i]);
  }

  guint tid = g_timeout_add (30000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached)
  {
    g_main_context_iteration (NULL, TRUE);
    bool all_loaded = true;
    for (int i = 1; i < icon_count; i += 2)
    {
      all_loaded &= results[i].got_callback;
      if (!all_loaded) break;
    }
    if (all_loaded) break;
  }

  for (i = 0; i < icon_count; i++)
  {
    if (i % 2)
    {
      EXPECT_TRUE(results[i].got_callback);
      EXPECT_TRUE(IsValidPixbuf(results[i].pixbuf));
    }
    else
    {
      EXPECT_FALSE(results[i].got_callback);
    }
  }

  g_source_remove (tid);
}


}
