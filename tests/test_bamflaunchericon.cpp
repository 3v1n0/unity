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
 */

#include <config.h>
#include <gmock/gmock.h>

#include <UnityCore/GLibWrapper.h>

#include "BamfLauncherIcon.h"
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
  BamfApplication* bamf_app_d;
  bamf_app_d = bamf_matcher_get_application_for_desktop_file(bamf_matcher,
                                                             BUILDDIR"/tests/data/no-icon.desktop",
                                                             TRUE);


  nux::ObjectPtr<launcher::BamfLauncherIcon> default_icon;
  default_icon = new launcher::BamfLauncherIcon(bamf_app_d);

  EXPECT_EQ(default_icon->icon_name.Get(), "application-default-icon");
}
}
