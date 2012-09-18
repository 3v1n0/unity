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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <config.h>
#include <gmock/gmock.h>

#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "test_utils.h"

using namespace unity;
using namespace unity::launcher;

namespace
{
const std::string USC_DESKTOP = BUILDDIR"/tests/data/applications/ubuntu-software-center.desktop";

struct TestSoftwareCenterLauncherIcon : testing::Test
{
  TestSoftwareCenterLauncherIcon()
    : bamf_matcher(bamf_matcher_get_default())
    , usc(bamf_matcher_get_application_for_desktop_file(bamf_matcher, USC_DESKTOP.c_str(), TRUE), glib::AddRef())
    , icon(usc, "", "")
  {}

  glib::Object<BamfMatcher> bamf_matcher;
  glib::Object<BamfApplication> usc;
  SoftwareCenterLauncherIcon icon;
};

TEST_F(TestSoftwareCenterLauncherIcon, Construction)
{
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
  EXPECT_EQ(icon.tooltip_text(), bamf_view_get_name(glib::object_cast<BamfView>(usc)));
}

TEST_F(TestSoftwareCenterLauncherIcon, Animate)
{
  ASSERT_FALSE(icon.IsVisible());

  Settings settings;
  panel::Style panel;
  nux::ObjectPtr<nux::BaseWindow> win(new nux::BaseWindow(""));
  nux::ObjectPtr<DNDCollectionWindow> cwin(new DNDCollectionWindow);
  nux::ObjectPtr<Launcher> launcher(new Launcher(win.GetPointer(), cwin));
  launcher->options = Options::Ptr(new Options);
  launcher->SetModel(LauncherModel::Ptr(new LauncherModel));

  icon.Animate(launcher, 1, 2);
  Utils::WaitForTimeoutMSec(500);

  EXPECT_TRUE(icon.IsVisible());
}

}
