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
#include <UnityCore/Variant.h>

#include "mock-application.h"
#include "FavoriteStore.h"
#include "MultiMonitor.h"
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
namespace
{
const std::string PRE_INSTALL_ICON = "sw-center-launcher-icon";
const std::string FINAL_ICON = "softwarecenter";
const std::string APP_NAME = "Ubuntu Software Center";
const std::string LOCAL_DATA_DIR = BUILDDIR"/tests/data";
const std::string USC_DESKTOP = LOCAL_DATA_DIR+"/applications/ubuntu-software-center.desktop";
const std::string USC_APP_INSTALL_DESKTOP = "/usr/share/app-install/desktop/software-center:ubuntu-software-center.desktop";
}

struct TestSoftwareCenterLauncherIcon : testmocks::TestUnityAppBase
{
  TestSoftwareCenterLauncherIcon()
     : usc(std::make_shared<MockApplication::Nice>(USC_APP_INSTALL_DESKTOP, FINAL_ICON, APP_NAME))
     , icon(usc, "/com/canonical/unity/test/object/path", PRE_INSTALL_ICON)
  {}

  struct MockSoftwareCenterLauncherIcon : SoftwareCenterLauncherIcon
  {
    MockSoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                   std::string const& aptdaemon_trans_id,
                                   std::string const& icon_path)
      : SoftwareCenterLauncherIcon(app, aptdaemon_trans_id, icon_path)
    {}

    void LauncherIconUnstick() { LauncherIcon::UnStick(); }

    using SoftwareCenterLauncherIcon::GetActualDesktopFileAfterInstall;
    using SoftwareCenterLauncherIcon::GetRemoteUri;
    using SoftwareCenterLauncherIcon::OnFinished;
    using SoftwareCenterLauncherIcon::OnPropertyChanged;
    using SoftwareCenterLauncherIcon::drag_window_;
  };

  nux::ObjectPtr<Launcher> CreateLauncher()
  {
    launcher_win = new MockableBaseWindow("");
    nux::ObjectPtr<Launcher> launcher(new Launcher(launcher_win.GetPointer()));
    launcher->options = Options::Ptr(new Options);
    launcher->SetModel(LauncherModel::Ptr(new LauncherModel));
    return launcher;
  }

  Settings settings;
  panel::Style panel;
  nux::ObjectPtr<MockableBaseWindow> launcher_win;
  MockApplication::Ptr usc;
  MockSoftwareCenterLauncherIcon icon;
};

TEST_F(TestSoftwareCenterLauncherIcon, Construction)
{
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_EQ(AbstractLauncherIcon::Position::FLOATING, icon.position());
  EXPECT_EQ("Waiting to install", icon.tooltip_text());
  EXPECT_EQ(PRE_INSTALL_ICON, icon.icon_name());
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformTrivial)
{
  // no transformation needed
  usc->desktop_file_ = USC_DESKTOP;
  EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), USC_DESKTOP);
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformAppInstall)
{
  // ensure that tranformation from app-install data desktop files works
  usc->desktop_file_ = "/usr/share/app-install/desktop/pkgname:kde4__afile.desktop";
   EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(),
             LOCAL_DATA_DIR+"/applications/kde4/afile.desktop");
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformSCAgent)
{
  // now simualte data coming from the sc-agent
  usc->desktop_file_ = "/tmp/software-center-agent:VP2W9M:ubuntu-software-center.desktop";
  EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), USC_DESKTOP);
}

// simulate a OnFinished signal from a /usr/share/app-install location
// and ensure that the remote uri is updated from temp location to
// the real location
TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedReplacesDesktopFile)
{
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  EXPECT_EQ(USC_DESKTOP, icon.DesktopFile());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedUpdatesRemoteURI)
{
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  ASSERT_EQ(USC_DESKTOP, icon.DesktopFile());
  EXPECT_EQ(FavoriteStore::URI_PREFIX_APP + USC_DESKTOP, icon.GetRemoteUri());
}

TEST_F(TestSoftwareCenterLauncherIcon, DisconnectsOldAppSignals)
{
  ASSERT_FALSE(usc->closed.empty());

  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  EXPECT_TRUE(usc->closed.empty());
  EXPECT_TRUE(usc->window_opened.empty());
  EXPECT_TRUE(usc->window_moved.empty());
  EXPECT_TRUE(usc->window_closed.empty());
  EXPECT_TRUE(usc->visible.changed.empty());
  EXPECT_TRUE(usc->active.changed.empty());
  EXPECT_TRUE(usc->running.changed.empty());
  EXPECT_TRUE(usc->urgent.changed.empty());
  EXPECT_TRUE(usc->desktop_file.changed.empty());
  EXPECT_TRUE(usc->title.changed.empty());
  EXPECT_TRUE(usc->icon.changed.empty());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedRemoveInvalidNewAppIcon)
{
  // Using an icon ptr, to get the removed signal to be properly emitted
  nux::ObjectPtr<MockSoftwareCenterLauncherIcon> icon_ptr(
    new MockSoftwareCenterLauncherIcon(usc, "/com/canonical/unity/test/object/path", PRE_INSTALL_ICON));

  bool removed = false;
  auto& app_manager = unity::ApplicationManager::Default();
  auto mock_app_managed = dynamic_cast<MockApplicationManager*>(&app_manager);

  EXPECT_CALL(*mock_app_managed, GetApplicationForDesktopFile(_)).WillOnce(Return(nullptr));
  icon_ptr->remove.connect([&removed] (AbstractLauncherIcon::Ptr const&) { removed = true; });

  icon_ptr->OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  Mock::VerifyAndClearExpectations(mock_app_managed);
  EXPECT_TRUE(removed);
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedSticksIcon)
{
  icon.LauncherIconUnstick();

  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  EXPECT_TRUE(icon.IsSticky());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedSavesIconPosition)
{
  icon.LauncherIconUnstick();

  bool saved = false;
  icon.position_saved.connect([&saved] {saved = true;});
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  ASSERT_TRUE(icon.IsSticky());
  EXPECT_TRUE(saved);
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedKeepsStickyStatus)
{
  icon.LauncherIconUnstick();

  bool saved = false;
  usc->sticky = true;
  icon.position_saved.connect([&saved] {saved = true;});
  ASSERT_FALSE(icon.IsSticky());

  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  ASSERT_TRUE(icon.IsSticky());
  EXPECT_TRUE(saved);
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedUpdatesTooltip)
{
  icon.tooltip_text = "FooText";
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  EXPECT_EQ(icon.tooltip_text(), usc->title());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedUpdatesIcon)
{
  icon.icon_name = "foo-icon";
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  EXPECT_EQ(icon.icon_name(), usc->icon());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedLogsEvent)
{
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _));
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
}

TEST_F(TestSoftwareCenterLauncherIcon, AnimateToInvalidPosition)
{
  EXPECT_FALSE(icon.Animate(CreateLauncher(), 1, 2));
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_EQ(PRE_INSTALL_ICON, icon.icon_name());
}

TEST_F(TestSoftwareCenterLauncherIcon, AnimateFromInvalidPosition)
{
  EXPECT_TRUE(icon.Animate(CreateLauncher(), 0, 0));
  EXPECT_TRUE(icon.IsVisible());
  EXPECT_EQ(PRE_INSTALL_ICON, icon.icon_name());
}

struct MultiMonitor : TestSoftwareCenterLauncherIcon, WithParamInterface<unsigned> {};
INSTANTIATE_TEST_CASE_P(TestSoftwareCenterLauncherIcon, MultiMonitor, Range<unsigned>(0, monitors::MAX, 1));

TEST_P(/*TestSoftwareCenterLauncherIcon*/MultiMonitor, Animate)
{
  auto launcher = CreateLauncher();
  launcher->monitor = GetParam();
  icon.SetCenter({1, 1, 0}, launcher->monitor());
  ASSERT_TRUE(icon.Animate(launcher, 2, 2));
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_TRUE(icon.icon_name().empty());

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_EQ(static_cast<int>(i) == launcher->monitor(), icon.IsVisibleOnMonitor(i));

  bool animated = false;
  ASSERT_TRUE(icon.drag_window_);
  icon.drag_window_->anim_completed.connect([&animated] { animated = true; });
  Utils::WaitUntilMSec(animated);
  ASSERT_TRUE(animated);

  EXPECT_EQ(PRE_INSTALL_ICON, icon.icon_name());
  EXPECT_FALSE(icon.drag_window_);

  for (unsigned i = 0; i < monitors::MAX; ++i)
    ASSERT_TRUE(icon.IsVisibleOnMonitor(i));

  EXPECT_TRUE(icon.IsVisible());
}

struct InstallProgress : TestSoftwareCenterLauncherIcon, WithParamInterface<int> {};
INSTANTIATE_TEST_CASE_P(TestSoftwareCenterLauncherIcon, InstallProgress, Range<int>(0, 99, 10));

TEST_P(/*TestSoftwareCenterLauncherIcon*/InstallProgress, InstallEmblems)
{
  ASSERT_EQ(icon.tooltip_text(), "Waiting to install");

  GVariant *progress = g_variant_new("i", GetParam());
  GVariant *params = g_variant_new("(sv)", "Progress", progress);

  icon.OnPropertyChanged(params);

  EXPECT_EQ("Installing…", icon.tooltip_text());
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::PROGRESS));
  EXPECT_FLOAT_EQ(GetParam()/100.0f, icon.GetProgress());

  g_variant_unref(params);
}

}

}
