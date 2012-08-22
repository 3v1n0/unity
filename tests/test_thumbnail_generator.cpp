// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <list>
#include <gmock/gmock.h>
using namespace testing;

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <unity-shared/ThumbnailGenerator.h>

#include <unity-protocol.h>
#include "test_utils.h"
#include "config.h"

using namespace unity;
using namespace unity::dash;

namespace
{

gboolean TimeoutReached (gpointer data)
{
  bool *b = static_cast<bool*>(data);

  *b = true;

  return FALSE;
}

struct LoadResult
{
  std::string return_string;
  bool got_callback;
  bool succeeded;

  LoadResult() : got_callback(false),succeeded(false) {}
  void ThumbnailReady(std::string const& result)
  {
    return_string = result;

    got_callback = true;
    succeeded = true;
  }
  void ThumbnailFailed(std::string const& result)
  {
    return_string = result;

    got_callback = true;
    succeeded = false;
  }
};

TEST(TestThumbnailGenerator, TestNoURIThumbnail)
{
  ThumbnailGenerator thumbnail_generator;
  ThumbnailNotifier::Ptr thumb = thumbnail_generator.GetThumbnail("", 256);
  EXPECT_TRUE(thumb == nullptr);
}

TEST(TestThumbnailGenerator, TestGetOneFileThumbnail)
{
  ThumbnailGenerator thumbnail_generator;

  LoadResult load_result;
  ThumbnailNotifier::Ptr thumb = thumbnail_generator.GetThumbnail("file://" PKGDATADIR "/switcher_background.png", 256);
  EXPECT_TRUE(thumb != nullptr);

  thumb->ready.connect(sigc::mem_fun(load_result, &LoadResult::ThumbnailReady));
  thumb->error.connect(sigc::mem_fun(load_result, &LoadResult::ThumbnailFailed));

  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (10000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !load_result.got_callback)
  {
    g_main_context_iteration (NULL, TRUE);
  }

  EXPECT_TRUE(load_result.succeeded);
  glib::Object<GIcon> icon(g_icon_new_for_string(load_result.return_string.c_str(), NULL));
  EXPECT_TRUE(G_IS_ICON(icon.RawPtr()));

  g_source_remove (tid);
}


TEST(TestThumbnailGenerator, TestGetManyFileThumbnail)
{
  srand ( time(NULL) );
  ThumbnailGenerator thumbnail_generator;

  const char* thumbs[] = { "file://" PKGDATADIR "/switcher_background.png" , "file://" PKGDATADIR "/star_highlight.png",
                          "file://" PKGDATADIR "/switcher_round_rect.png", "file://" PKGDATADIR "/switcher_corner.png",
                          "file://" PKGDATADIR "/switcher_top.png", "file://" PKGDATADIR "/switcher_left.png",
                          "file://" PKGDATADIR "/dash_bottom_left_corner.png", "file://" PKGDATADIR "/dash_bottom_right_corner.png"};

  std::vector<LoadResult> results;
  std::vector< ThumbnailNotifier::Ptr> notifiers;

  // 100 times should be good
  int load_count = 100;
  results.resize (load_count);
  notifiers.resize (load_count);

  for (int i = 0; i < load_count; i++)
  {
    notifiers[i] = thumbnail_generator.GetThumbnail(thumbs[rand() % (sizeof(thumbs) / sizeof(char*))], 256);
    EXPECT_TRUE(notifiers[i] != nullptr);

    notifiers[i]->ready.connect(sigc::mem_fun(results[i], &LoadResult::ThumbnailReady));
    notifiers[i]->error.connect(sigc::mem_fun(results[i], &LoadResult::ThumbnailFailed));
  }

  // disconnect every other handler (and especially the first one)
  for (int i = 0; i < load_count; i += 2)
  {
    notifiers[i]->Cancel();
  }


  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (30000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached)
  {
    g_main_context_iteration (NULL, TRUE);
    bool all_loaded = true;
    bool any_loaded = false;
    for (int i = 1; i < load_count; i += 2)
    {
      all_loaded &= results[i].got_callback;
      any_loaded |= results[i].got_callback;
      if (!all_loaded) break;
    }
    if (all_loaded) break;
  }
  
  for (int i = 0; i < load_count; i++)
  {
    if (i % 2)
    {
      EXPECT_TRUE(results[i].got_callback);
      EXPECT_TRUE(results[i].succeeded);
      glib::Object<GIcon> icon(g_icon_new_for_string(results[i].return_string.c_str(), NULL));
      EXPECT_TRUE(G_IS_ICON(icon.RawPtr()));
    }
    else
    {
      EXPECT_FALSE(results[i].got_callback);
    }
  }

  g_source_remove (tid);
}


TEST(TestThumbnailGenerator, TestGetOneGIcon)
{
  ThumbnailGenerator thumbnail_generator;

  LoadResult load_result;
  ThumbnailNotifier::Ptr thumb = thumbnail_generator.GetThumbnail("file:///home", 256);
  EXPECT_TRUE(thumb != nullptr);

  thumb->ready.connect(sigc::mem_fun(load_result, &LoadResult::ThumbnailReady));
  thumb->error.connect(sigc::mem_fun(load_result, &LoadResult::ThumbnailFailed));

  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (10000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached && !load_result.got_callback)
  {
    g_main_context_iteration (NULL, TRUE);
  }

  EXPECT_TRUE(load_result.succeeded);
  glib::Object<GIcon> icon(g_icon_new_for_string(load_result.return_string.c_str(), NULL));
  EXPECT_TRUE(G_IS_ICON(icon.RawPtr()));

  g_source_remove (tid);
}


TEST(TestThumbnailGenerator, TestGetManyGIcon)
{
  srand ( time(NULL) );
  ThumbnailGenerator thumbnail_generator;

  const char* thumbs[] = { "file:///home",
                          "file:///usr",
                          "file:///bin/bash",
                          "file:///usr/bin/unity"};

  std::vector<LoadResult> results;
  std::vector< ThumbnailNotifier::Ptr> notifiers;

  // 100 times should be good
  int load_count = 100;
  results.resize (load_count);
  notifiers.resize (load_count);

  for (int i = 0; i < load_count; i++)
  {
    notifiers[i] = thumbnail_generator.GetThumbnail(thumbs[rand() % (sizeof(thumbs) / sizeof(char*))], 256);
    EXPECT_TRUE(notifiers[i] != nullptr);

    notifiers[i]->ready.connect(sigc::mem_fun(results[i], &LoadResult::ThumbnailReady));
    notifiers[i]->error.connect(sigc::mem_fun(results[i], &LoadResult::ThumbnailFailed));
  }

  // disconnect every other handler (and especially the first one)
  for (int i = 0; i < load_count; i += 2)
  {
    notifiers[i]->Cancel();
  }

  volatile bool timeout_reached = false;
  guint tid = g_timeout_add (30000, TimeoutReached, (gpointer)(&timeout_reached));
  while (!timeout_reached)
  {
    g_main_context_iteration (NULL, TRUE);
    bool all_loaded = true;
    bool any_loaded = false;
    for (int i = 1; i < load_count; i += 2)
    {
      all_loaded &= results[i].got_callback;
      any_loaded |= results[i].got_callback;
      if (!all_loaded) break;
    }
    if (all_loaded) break;
  }
  
  for (int i = 0; i < load_count; i++)
  {
    if (i % 2)
    {
      EXPECT_TRUE(results[i].got_callback);
      EXPECT_TRUE(results[i].succeeded);
      glib::Object<GIcon> icon(g_icon_new_for_string(results[i].return_string.c_str(), NULL));
      EXPECT_TRUE(G_IS_ICON(icon.RawPtr()));
    }
    else
    {
      EXPECT_FALSE(results[i].got_callback);
    }
  }

  g_source_remove (tid);
}


}
