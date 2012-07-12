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
 * Authored by: Andrea Azzarone <azzarone@gmail.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <config.h>
#include <gmock/gmock.h>

#include <UnityCore/GLibWrapper.h>

#include "BamfLauncherIcon.h"

#include "unity-shared/PluginAdapter.h"
#include <gio/gdesktopappinfo.h>
#include <unistd.h>

using namespace unity;

namespace
{

class TestBamfLauncherIcon : public testing::Test
{
public:
  virtual void SetUp()
  {
    bamf_matcher = bamf_matcher_get_default();
    bamf_app = bamf_matcher_get_application_for_desktop_file(bamf_matcher,
                                                             BUILDDIR"/tests/data/ubuntu-software-center.desktop",
                                                             TRUE);

    usc_icon = new launcher::BamfLauncherIcon(bamf_app);

  }

  virtual void TearDown()
  {
  }

  glib::Object<BamfMatcher> bamf_matcher;
  BamfApplication* bamf_app;
  nux::ObjectPtr<launcher::BamfLauncherIcon> usc_icon;

};

TEST_F(TestBamfLauncherIcon, TestCustomBackgroundColor)
{
  nux::Color const& color = usc_icon->BackgroundColor();

  EXPECT_EQ(color.red, 0xaa / 255.0f);
  EXPECT_EQ(color.green, 0xbb / 255.0f);
  EXPECT_EQ(color.blue, 0xcc / 255.0f);
  EXPECT_EQ(color.alpha, 0xff / 255.0f);
}

TEST_F(TestBamfLauncherIcon, TestDefaultIcon)
{
  glib::Error error;
  BamfApplication* app;
  nux::ObjectPtr<launcher::BamfLauncherIcon> default_icon;

  glib::Object<GDesktopAppInfo> desktopInfo(g_desktop_app_info_new_from_filename(BUILDDIR"/tests/data/no-icon.desktop"));
  auto appInfo = glib::object_cast<GAppInfo>(desktopInfo);

  g_app_info_launch(appInfo, nullptr, nullptr, &error);

  if (error)
        g_warning("%s\n", error.Message().c_str());
  EXPECT_FALSE(error);

  for (int i = 0; i < 5 && !bamf_matcher_application_is_running(bamf_matcher, BUILDDIR"/tests/data/no-icon.desktop"); i++)
    sleep(1);
  EXPECT_TRUE(bamf_matcher_application_is_running(bamf_matcher, BUILDDIR"/tests/data/no-icon.desktop"));

  app = bamf_matcher_get_active_application(bamf_matcher);
  default_icon = new launcher::BamfLauncherIcon(app);

  GList* children, *l;
  children = bamf_view_get_children(BAMF_VIEW(app));

  for (l = children; l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    Window xid = bamf_window_get_xid(static_cast<BamfWindow*>(l->data));
    PluginAdapter::Default()->Close(xid);
  }
  g_list_free(children);

  EXPECT_EQ(default_icon->icon_name.Get(), "application-default-icon");
}

}
