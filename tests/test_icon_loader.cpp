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
#include "test_utils.h"

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
  bool disconnected;

  LoadResult() : pixbuf(NULL), got_callback(false), disconnected(false) {}
  void IconLoaded(std::string const& icon_name, int max_width, int max_height,
                  glib::Object<GdkPixbuf> const& buf)
  {
    pixbuf = buf;

    got_callback = true;
  }
};

void CheckResults(std::vector<LoadResult> const& results)
{
  Utils::WaitUntilMSec([&results] {
    bool got_all = true;
    for (auto const& result : results)
    {
      got_all = (result.got_callback == !result.disconnected);

      if (!got_all)
        break;
    }

    return got_all;
  });

  for (auto const& result : results)
  {
    if (!result.disconnected)
    {
      ASSERT_TRUE(result.got_callback);
      ASSERT_TRUE(IsValidPixbuf(result.pixbuf));
    }
    else
    {
      ASSERT_FALSE(result.got_callback);
    }
  }
}


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

  icon_loader.LoadFromIconName("gedit-icon", -1, 48, sigc::mem_fun(load_result,
        &LoadResult::IconLoaded));

  Utils::WaitUntilMSec(load_result.got_callback);
  EXPECT_TRUE(load_result.got_callback);
  EXPECT_TRUE(IsValidPixbuf(load_result.pixbuf));
}

TEST(TestIconLoader, TestGetAnnotatedIcon)
{
  LoadResult load_result;
  IconLoader& icon_loader = IconLoader::GetDefault();

  icon_loader.LoadFromGIconString(". UnityProtocolAnnotatedIcon %7B'base-icon':%20%3C'gedit-icon'%3E,%20'ribbon':%20%3C'foo'%3E%7D", -1, 48, sigc::mem_fun(load_result,
        &LoadResult::IconLoaded));

  Utils::WaitUntilMSec(load_result.got_callback);
  EXPECT_TRUE(load_result.got_callback);
  EXPECT_TRUE(IsValidPixbuf(load_result.pixbuf));
}

TEST(TestIconLoader, TestGetOneIconManyTimes)
{
  std::vector<LoadResult> results;
  std::vector<int> handles;
  IconLoader& icon_loader = IconLoader::GetDefault();
  int i, load_count;

  // 100 times should be good
  load_count = 100;
  results.resize (load_count);
  handles.resize (load_count);

  // careful, don't use the same icon as in previous tests, otherwise it'll
  // be cached already!
  for (int i = 0; i < load_count; i++)
  {
    handles[i] = icon_loader.LoadFromIconName("web-browser", -1, 48,
        sigc::mem_fun(results[i], &LoadResult::IconLoaded));
  }

  // disconnect every other handler (and especially the first one)
  for (i = 0; i < load_count; i += 2)
  {
    icon_loader.DisconnectHandle(handles[i]);
    results[i].disconnected = true;
  }

  CheckResults(results);
}

// Disabled until we have the new thread safe lp:fontconfig
TEST(TestIconLoader, DISABLED_TestGetManyIcons)
{
  std::vector<LoadResult> results;
  IconLoader& icon_loader = IconLoader::GetDefault();
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
    icon_loader.LoadFromIconName(icon_name, -1, 48, sigc::mem_fun(results[i++],
        &LoadResult::IconLoaded));
    if (i >= icon_count) break;
  }

  CheckResults(results);
}

TEST(TestIconLoader, TestCancelSome)
{
  std::vector<LoadResult> results;
  std::vector<int> handles;
  IconLoader& icon_loader = IconLoader::GetDefault();
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
    int handle = icon_loader.LoadFromIconName(icon_name, -1, 48, sigc::mem_fun(
          results[i], &LoadResult::IconLoaded));
    handles[i++] = handle;
    if (i >= icon_count) break;
  }

  // disconnect every other handler
  for (i = 0; i < icon_count; i += 2)
  {
    icon_loader.DisconnectHandle(handles[i]);
    results[i].disconnected = true;
  }

  CheckResults(results);
}


}
