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
 *              Michael Vogt <mvo@ubuntu.com>
 *
 * Run standalone with:
 * cd build && make test-gtest && ./test-gtest --gtest_filter=TestSoftwareCenterLauncherIcon.*
 */

#include <config.h>
#include <gmock/gmock.h>

#include "mock-application.h"
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "test_utils.h"

using namespace testmocks;
using namespace unity;
using namespace unity::launcher;

namespace unity
{
namespace launcher
{
const std::string LOCAL_DATA_DIR = BUILDDIR"/tests/data";
const std::string USC_DESKTOP = LOCAL_DATA_DIR+"/applications/ubuntu-software-center.desktop";
const std::string USC_APP_INSTALL_DESKTOP = "/usr/share/app-install/desktop/software-center:ubuntu-software-center.desktop";

class MockSoftwareCenterLauncherIcon : public SoftwareCenterLauncherIcon
{
public:
   MockSoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                  std::string const& aptdaemon_trans_id,
                                  std::string const& icon_path)
      : SoftwareCenterLauncherIcon(app, aptdaemon_trans_id, icon_path)
   {}

   using SoftwareCenterLauncherIcon::GetActualDesktopFileAfterInstall;
   using SoftwareCenterLauncherIcon::_desktop_file;
   using SoftwareCenterLauncherIcon::GetRemoteUri;
   using SoftwareCenterLauncherIcon::OnFinished;
};

struct TestSoftwareCenterLauncherIcon : testing::Test
{
public:
  TestSoftwareCenterLauncherIcon()
     : usc(std::make_shared<MockApplication::Nice>(USC_APP_INSTALL_DESKTOP, "softwarecenter"))
     , icon(usc, "/com/canonical/unity/test/object/path", "")
  {}

  ApplicationPtr usc;
  MockSoftwareCenterLauncherIcon icon;
};

TEST_F(TestSoftwareCenterLauncherIcon, Construction)
{
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::FLOATING);
  EXPECT_EQ(icon.tooltip_text(), "Waiting to install");
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformTrivial)
{
   // no transformation needed
  icon._desktop_file = USC_DESKTOP;
  EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), USC_DESKTOP);
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformAppInstall)
{
   // ensure that tranformation from app-install data desktop files works
   icon._desktop_file = "/usr/share/app-install/desktop/pkgname:kde4__afile.desktop";
   EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), 
             BUILDDIR"/tests/data/applications/kde4/afile.desktop");
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformSCAgent)
{
   // now simualte data coming from the sc-agent
   icon._desktop_file = "/tmp/software-center-agent:VP2W9M:ubuntu-software-center.desktop";
   EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), USC_DESKTOP);
}

// simulate a OnFinished signal from a /usr/share/app-install location
// and ensure that the remote uri is updated from temp location to
// the real location
TEST_F(TestSoftwareCenterLauncherIcon, OnFinished)
{   
   // now simulate that the install was successful
   GVariant *params = g_variant_new("(s)", "exit-success");
   icon.OnFinished(params);

   // and verify that both the desktop file and the remote uri gets updated

   EXPECT_EQ(icon._desktop_file, USC_DESKTOP);
   EXPECT_EQ(icon.GetRemoteUri(), "application://ubuntu-software-center.desktop");
   EXPECT_TRUE(usc->closed.empty());

   g_variant_unref(params);
}

TEST_F(TestSoftwareCenterLauncherIcon, Animate)
{
  ASSERT_FALSE(icon.IsVisible());

  Settings settings;
  panel::Style panel;
  nux::ObjectPtr<MockableBaseWindow> win(new MockableBaseWindow(""));
  nux::ObjectPtr<Launcher> launcher(new Launcher(win.GetPointer()));
  launcher->options = Options::Ptr(new Options);
  launcher->SetModel(LauncherModel::Ptr(new LauncherModel));

  icon.Animate(launcher, 1, 2);
  Utils::WaitForTimeoutMSec(500);

  EXPECT_TRUE(icon.IsVisible());
}

}

}
