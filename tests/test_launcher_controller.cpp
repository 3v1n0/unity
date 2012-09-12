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

#include <gmock/gmock.h>
#include "test_uscreen_mock.h"

#include "FavoriteStore.h"
#include "LauncherController.h"
#include "LauncherControllerPrivate.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "test_utils.h"
#include "test_mock_devices.h"

using namespace unity::launcher;
using namespace testing;

namespace unity
{

struct MockFavoriteStore : FavoriteStore
{
  MockFavoriteStore()
  {
    fav_list_ = { "application://" BUILDDIR "/tests/data/ubuntuone-installer.desktop",
                  "application://" BUILDDIR "/tests/data/ubuntu-software-center.desktop",
                  "application://" BUILDDIR "/tests/data/update-manager.desktop" };
  }

  FavoriteList const& GetFavorites()
  {
    return fav_list_;
  }

  void AddFavorite(std::string const& icon_uri, int position)
  {
    if (!IsValidFavoriteUri(icon_uri))
      return;

    if (position < 0)
      fav_list_.push_back(icon_uri);
    else
    {
      auto it = fav_list_.begin();
      std::advance(it, position);
      fav_list_.insert(it, icon_uri);
    }
  }

  void RemoveFavorite(std::string const& icon_uri)
  {
    fav_list_.remove(icon_uri);
  }

  void MoveFavorite(std::string const& icon_uri, int position)
  {
    RemoveFavorite(icon_uri);
    AddFavorite(icon_uri, position);
  }

  bool IsFavorite(std::string const& icon_uri) const
  {
    return std::find(fav_list_.begin(), fav_list_.end(), icon_uri) != fav_list_.end();
  }

  int FavoritePosition(std::string const& icon_uri) const
  {
    auto it = std::find(fav_list_.begin(), fav_list_.end(), icon_uri);

    if (it != fav_list_.end())
      return std::distance(fav_list_.begin(), it);

    return -1;
  }

  void SetFavorites(FavoriteList const& icon_uris)
  {
    fav_list_ = icon_uris;
  }

private:
  FavoriteList fav_list_;
};

class MockBamfLauncherIcon : public BamfLauncherIcon
{
public:
  MockBamfLauncherIcon(BamfApplication* app)
    : BamfLauncherIcon(app) {}

  MOCK_METHOD0(UnStick, void());
  MOCK_METHOD0(Quit, void());
};

namespace launcher
{
struct TestLauncherController : public testing::Test
{
  virtual void SetUp()
  {
    lc.multiple_launchers = true;
  }

protected:
  struct MockLauncherController : Controller
  {
    Controller::Impl* Impl() const { return pimpl.get(); }
  };

  MockUScreen uscreen;
  Settings settings;
  panel::Style panel_style;
  MockFavoriteStore favorite_store;
  MockLauncherController lc;
};
}

TEST_F(TestLauncherController, Construction)
{
  EXPECT_NE(lc.options(), nullptr);
  EXPECT_TRUE(lc.multiple_launchers());
  ASSERT_EQ(lc.launchers().size(), 1);
  EXPECT_EQ(lc.launcher().monitor(), 0);
}

TEST_F(TestLauncherController, MultimonitorMultipleLaunchers)
{
  lc.multiple_launchers = true;
  uscreen.SetupFakeMultiMonitor();

  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  for (int i = 0; i < max_num_monitors; ++i)
  {
    EXPECT_EQ(lc.launchers()[i]->monitor(), i);
  }
}

TEST_F(TestLauncherController, MultimonitorSingleLauncher)
{
  lc.multiple_launchers = false;
  uscreen.SetupFakeMultiMonitor(0, false);

  for (int i = 0; i < max_num_monitors; ++i)
  {
    uscreen.SetPrimary(i);
    ASSERT_EQ(lc.launchers().size(), 1);
    EXPECT_EQ(lc.launcher().monitor(), i);
  }
}

TEST_F(TestLauncherController, MultimonitorSwitchToMultipleLaunchers)
{
  lc.multiple_launchers = false;
  uscreen.SetupFakeMultiMonitor();

  ASSERT_EQ(lc.launchers().size(), 1);

  lc.multiple_launchers = true;
  EXPECT_EQ(lc.launchers().size(), max_num_monitors);
}

TEST_F(TestLauncherController, MultimonitorSwitchToSingleLauncher)
{
  lc.multiple_launchers = true;
  int primary = 3;
  uscreen.SetupFakeMultiMonitor(primary);

  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  lc.multiple_launchers = false;
  EXPECT_EQ(lc.launchers().size(), 1);
  EXPECT_EQ(lc.launcher().monitor(), primary);
}

TEST_F(TestLauncherController, MultimonitorSwitchToSingleMonitor)
{
  uscreen.SetupFakeMultiMonitor();
  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  uscreen.Reset();
  EXPECT_EQ(lc.launchers().size(), 1);
  EXPECT_EQ(lc.launcher().monitor(), 0);
}

TEST_F(TestLauncherController, MultimonitorRemoveMiddleMonitor)
{
  uscreen.SetupFakeMultiMonitor();
  ASSERT_EQ(lc.launchers().size(), max_num_monitors);

  std::vector<nux::Geometry> &monitors = uscreen.GetMonitors();
  monitors.erase(monitors.begin() + monitors.size()/2);
  uscreen.changed.emit(uscreen.GetPrimaryMonitor(), uscreen.GetMonitors());
  ASSERT_EQ(lc.launchers().size(), max_num_monitors - 1);

  for (int i = 0; i < max_num_monitors - 1; ++i)
    EXPECT_EQ(lc.launchers()[i]->monitor(), i);
}

TEST_F(TestLauncherController, SingleMonitorSwitchToMultimonitor)
{
  ASSERT_EQ(lc.launchers().size(), 1);

  uscreen.SetupFakeMultiMonitor();

  EXPECT_EQ(lc.launchers().size(), max_num_monitors);
}

TEST_F(TestLauncherController, MultiMonitorEdgeBarrierSubscriptions)
{
  uscreen.SetupFakeMultiMonitor();

  for (int i = 0; i < max_num_monitors; ++i)
    ASSERT_EQ(lc.Impl()->edge_barriers_.GetSubscriber(i), lc.launchers()[i].GetPointer());
}

TEST_F(TestLauncherController, SingleMonitorEdgeBarrierSubscriptionsUpdates)
{
  lc.multiple_launchers = false;
  uscreen.SetupFakeMultiMonitor(0, false);

  for (int i = 0; i < max_num_monitors; ++i)
  {
    uscreen.SetPrimary(i);

    for (int j = 0; j < max_num_monitors; ++j)
    {
      if (j == i)
      {
        ASSERT_EQ(lc.Impl()->edge_barriers_.GetSubscriber(j), &lc.launcher());
      }
      else
      {
        ASSERT_EQ(lc.Impl()->edge_barriers_.GetSubscriber(j), nullptr);
      }
    }
  }
}

TEST_F(TestLauncherController, OnlyUnstickIconOnFavoriteRemoval)
{
  const std::string USC_DESKTOP = BUILDDIR"/tests/data/ubuntu-software-center.desktop";

  glib::Object<BamfMatcher> matcher(bamf_matcher_get_default());

  auto bamf_app = bamf_matcher_get_application_for_desktop_file(matcher, USC_DESKTOP.c_str(), TRUE);
  MockBamfLauncherIcon *bamf_icon = new MockBamfLauncherIcon(bamf_app);
  lc.Impl()->model_->AddIcon(AbstractLauncherIcon::Ptr(bamf_icon));

  EXPECT_CALL(*bamf_icon, UnStick());
  EXPECT_CALL(*bamf_icon, Quit()).Times(0);

  favorite_store.favorite_removed.emit(FavoriteStore::URI_PREFIX_APP + USC_DESKTOP);
}

TEST_F(TestLauncherController, EnabledStrutsByDefault)
{
  EXPECT_EQ(lc.launcher().options()->hide_mode, LAUNCHER_HIDE_NEVER);
  EXPECT_TRUE(lc.launcher().GetParent()->InputWindowStrutsEnabled());
}

TEST_F(TestLauncherController, EnabledStrutsOnNeverHide)
{
  lc.multiple_launchers = true;
  uscreen.SetupFakeMultiMonitor();
  lc.options()->hide_mode = LAUNCHER_HIDE_NEVER;

  for (int i = 0; i < max_num_monitors; ++i)
    ASSERT_TRUE(lc.launchers()[i]->GetParent()->InputWindowStrutsEnabled());
}

TEST_F(TestLauncherController, DisabledStrutsOnAutoHide)
{
  lc.multiple_launchers = true;
  uscreen.SetupFakeMultiMonitor();
  lc.options()->hide_mode = LAUNCHER_HIDE_AUTOHIDE;

  for (int i = 0; i < max_num_monitors; ++i)
    ASSERT_FALSE(lc.launchers()[i]->GetParent()->InputWindowStrutsEnabled());
}

TEST_F(TestLauncherController, EnabledStrutsAddingNewLaunchersOnAutoHide)
{
  // This makes the controller to add multiple launchers
  lc.multiple_launchers = true;
  lc.options()->hide_mode = LAUNCHER_HIDE_NEVER;
  uscreen.SetupFakeMultiMonitor();

  // This makes the controller to remove unneeded launchers
  lc.multiple_launchers = false;

  // This makes the controller to add again new launchers
  lc.multiple_launchers = true;

  for (int i = 0; i < max_num_monitors; ++i)
    ASSERT_TRUE(lc.launchers()[i]->GetParent()->InputWindowStrutsEnabled());
}

TEST_F(TestLauncherController, DisabledStrutsAddingNewLaunchersOnNeverHide)
{
  // This makes the controller to add multiple launchers
  lc.multiple_launchers = true;
  lc.options()->hide_mode = LAUNCHER_HIDE_AUTOHIDE;
  uscreen.SetupFakeMultiMonitor();

  // This makes the controller to remove unneeded launchers
  lc.multiple_launchers = false;

  // This makes the controller to add again new launchers
  lc.multiple_launchers = true;

  for (int i = 0; i < max_num_monitors; ++i)
    ASSERT_FALSE(lc.launchers()[i]->GetParent()->InputWindowStrutsEnabled());
}

TEST_F(TestLauncherController, CreateFavoriteInvalid)
{
  auto const& fav = lc.Impl()->CreateFavoriteIcon("InvalidUri");

  EXPECT_FALSE(fav.IsValid());
}

TEST_F(TestLauncherController, CreateFavoriteDesktopFile)
{
  std::string desktop_file = BUILDDIR "/tests/data/bzr-handle-patch.desktop";
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop_file;
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);

  ASSERT_TRUE(fav.IsValid());
  EXPECT_EQ(fav->GetIconType(), AbstractLauncherIcon::IconType::APPLICATION);
  EXPECT_EQ(fav->DesktopFile(), desktop_file);
  EXPECT_EQ(fav->RemoteUri(), icon_uri);
  EXPECT_TRUE(fav->IsSticky());
  EXPECT_NE(dynamic_cast<BamfLauncherIcon*>(fav.GetPointer()), nullptr);
}

TEST_F(TestLauncherController, CreateFavoriteInvalidDesktopFile)
{
  // This desktop file has already been added as favorite, so it is invalid
  std::string desktop_file = *(favorite_store.GetFavorites().begin());
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop_file;
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);

  EXPECT_FALSE(fav.IsValid());
}

TEST_F(TestLauncherController, CreateFavoriteDevice)
{
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& icons = lc.Impl()->device_section_.GetIcons();
  auto const& device_icon = *(icons.begin());

  ASSERT_TRUE(device_icon.IsValid());
  ASSERT_FALSE(device_icon->IsSticky());

  auto const& fav = lc.Impl()->CreateFavoriteIcon(device_icon->RemoteUri());

  ASSERT_TRUE(fav.IsValid());
  EXPECT_EQ(fav, device_icon);
  EXPECT_EQ(fav->GetIconType(), AbstractLauncherIcon::IconType::DEVICE);
  EXPECT_EQ(fav->RemoteUri(), device_icon->RemoteUri());
  EXPECT_TRUE(fav->IsSticky());
  EXPECT_NE(dynamic_cast<VolumeLauncherIcon*>(fav.GetPointer()), nullptr);
}

TEST_F(TestLauncherController, CreateFavoriteInvalidDevice)
{
  auto const& fav = lc.Impl()->CreateFavoriteIcon(FavoriteStore::URI_PREFIX_APP + "invalid-uuid");

  EXPECT_FALSE(fav.IsValid());
}

}

