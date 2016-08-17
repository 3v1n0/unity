/*
 * Copyright 2012,2015 Canonical Ltd.
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
#include "SoftwareCenterLauncherIcon.h"
#include "Launcher.h"
#include "PanelStyle.h"
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
const std::string FINAL_ICON = "org.gnome.Software";
const std::string APP_NAME = "Software";
const std::string LOCAL_DATA_DIR = BUILDDIR"/tests/data";
const std::string GS_DESKTOP = LOCAL_DATA_DIR+"/applications/org.gnome.Software.desktop";
const std::string GS_APP_INSTALL_DESKTOP = "org.gnome.Software.desktop";
}

struct TestSoftwareCenterLauncherIcon : testmocks::TestUnityAppBase
{
  TestSoftwareCenterLauncherIcon()
     : gs(std::make_shared<MockApplication::Nice>(GS_APP_INSTALL_DESKTOP, FINAL_ICON, APP_NAME))
     , icon(gs, "/com/canonical/unity/test/object/path")
  {}

  struct MockSoftwareCenterLauncherIcon : SoftwareCenterLauncherIcon
  {
    MockSoftwareCenterLauncherIcon(ApplicationPtr const& app,
                                   std::string const& aptdaemon_trans_id)
      : WindowedLauncherIcon(IconType::APPLICATION)
      , SoftwareCenterLauncherIcon(app, aptdaemon_trans_id)
    {}

    void LauncherIconUnstick() { LauncherIcon::UnStick(); }

    using SoftwareCenterLauncherIcon::GetActualDesktopFileAfterInstall;
    using SoftwareCenterLauncherIcon::OnFinished;
    using SoftwareCenterLauncherIcon::OnPropertyChanged;
    using LauncherIcon::GetRemoteUri;
  };

  nux::ObjectPtr<Launcher> CreateLauncher()
  {
    launcher_win = new MockableBaseWindow("");
    nux::ObjectPtr<Launcher> launcher(new Launcher(launcher_win.GetPointer()));
    launcher->options = Options::Ptr(new Options);
    launcher->SetModel(LauncherModel::Ptr(new LauncherModel));
    return launcher;
  }

  panel::Style panel;
  nux::ObjectPtr<MockableBaseWindow> launcher_win;
  MockApplication::Ptr gs;
  MockSoftwareCenterLauncherIcon icon;
};

TEST_F(TestSoftwareCenterLauncherIcon, Construction)
{
  EXPECT_FALSE(icon.IsVisible());
  EXPECT_TRUE(icon.IsSticky());
  EXPECT_EQ("Waiting to install", icon.tooltip_text());
}

TEST_F(TestSoftwareCenterLauncherIcon, DesktopFileTransformTrivial)
{
  // no transformation needed
  gs->desktop_file_ = GS_DESKTOP;
  EXPECT_EQ(icon.GetActualDesktopFileAfterInstall(), GS_DESKTOP);
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedReplacesDesktopFile)
{
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
  EXPECT_EQ(GS_DESKTOP, icon.DesktopFile());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedUpdatesRemoteURI)
{
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  ASSERT_EQ(GS_DESKTOP, icon.DesktopFile());
  EXPECT_EQ(FavoriteStore::URI_PREFIX_APP + GS_DESKTOP, icon.GetRemoteUri());
}

TEST_F(TestSoftwareCenterLauncherIcon, DisconnectsOldAppSignals)
{
  ASSERT_FALSE(gs->closed.empty());

  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));

  EXPECT_TRUE(gs->closed.empty());
  EXPECT_TRUE(gs->window_opened.empty());
  EXPECT_TRUE(gs->window_moved.empty());
  EXPECT_TRUE(gs->window_closed.empty());
  EXPECT_TRUE(gs->visible.changed.empty());
  EXPECT_TRUE(gs->active.changed.empty());
  EXPECT_TRUE(gs->running.changed.empty());
  EXPECT_TRUE(gs->urgent.changed.empty());
  EXPECT_TRUE(gs->desktop_file.changed.empty());
  EXPECT_TRUE(gs->title.changed.empty());
  EXPECT_TRUE(gs->icon.changed.empty());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedRemoveInvalidNewAppIcon)
{
  // Using an icon ptr, to get the removed signal to be properly emitted
  nux::ObjectPtr<MockSoftwareCenterLauncherIcon> icon_ptr(
    new MockSoftwareCenterLauncherIcon(gs, "/com/canonical/unity/test/object/path"));

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
  gs->sticky = true;
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
  EXPECT_EQ(icon.tooltip_text(), gs->title());
}

TEST_F(TestSoftwareCenterLauncherIcon, OnFinishedLogsEvent)
{
  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::ACCESS, _));
  icon.OnFinished(glib::Variant(g_variant_new("(s)", "exit-success")));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"


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

#pragma GCC diagnostic pop

}

}
