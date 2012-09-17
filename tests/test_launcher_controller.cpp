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
#include "ExpoLauncherIcon.h"
#include "DesktopLauncherIcon.h"
#include "MockLauncherIcon.h"
#include "BFBLauncherIcon.h"
#include "HudLauncherIcon.h"
#include "TrashLauncherIcon.h"
#include "VolumeLauncherIcon.h"
#include "SoftwareCenterLauncherIcon.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "test_utils.h"
#include "test_mock_devices.h"

using namespace unity::launcher;
using namespace testing;

namespace unity
{

namespace
{
namespace places
{
const std::string APPS_URI = "unity://running-apps";
const std::string DEVICES_URI = "unity://devices";
}

namespace app
{
 const std::string UBUNTU_ONE = BUILDDIR "/tests/data/ubuntuone-installer.desktop";
 const std::string SW_CENTER = BUILDDIR "/tests/data/ubuntu-software-center.desktop";
 const std::string UPDATE_MANAGER = BUILDDIR "/tests/data/update-manager.desktop";
 const std::string BZR_HANDLE_PATCH = BUILDDIR "/tests/data/bzr-handle-patch.desktop";
 const std::string NO_ICON = BUILDDIR "/tests/data/no-icon.desktop";
}
}

struct MockFavoriteStore : FavoriteStore
{
  MockFavoriteStore()
  {
    fav_list_ = { FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE,
                  FavoriteStore::URI_PREFIX_APP + app::SW_CENTER,
                  FavoriteStore::URI_PREFIX_APP + app::UPDATE_MANAGER };
  }

  FavoriteList const& GetFavorites() const
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

  void ClearFavorites()
  {
    fav_list_.clear();
  }

private:
  FavoriteList fav_list_;
};

struct MockBamfLauncherIcon : public BamfLauncherIcon
{
  typedef nux::ObjectPtr<MockBamfLauncherIcon> Ptr;
  typedef bool Fake;

  MockBamfLauncherIcon(Fake = true, std::string const& remote_uri = "")
    : BamfLauncherIcon(static_cast<BamfApplication*>(g_object_new(BAMF_TYPE_APPLICATION, nullptr)))
    , remote_uri_(remote_uri)
  {
    InitMock();
    SetQuirk(Quirk::VISIBLE, true);
  }

  explicit MockBamfLauncherIcon(BamfApplication* app)
    : BamfLauncherIcon(app)
  {
    InitMock();
  }

  MockBamfLauncherIcon(std::string const& desktop_file)
    : BamfLauncherIcon(bamf_matcher_get_application_for_desktop_file(bamf_matcher_get_default(), desktop_file.c_str(), TRUE))
  {
    InitMock();
  }

  void InitMock()
  {
    ON_CALL(*this, Stick(_)).WillByDefault(Invoke(this, &MockBamfLauncherIcon::ReallyStick));
  }

  std::string GetRemoteUri()
  {
    if (remote_uri_.empty())
      return BamfLauncherIcon::GetRemoteUri();
    else
      return FavoriteStore::URI_PREFIX_APP + remote_uri_;
  }

  bool IsSticky() const
  {
    if (remote_uri_.empty())
      return BamfLauncherIcon::IsSticky();
    else
      return SimpleLauncherIcon::IsSticky();
  }

  void ReallyStick(bool save) { BamfLauncherIcon::Stick(save); }

  MOCK_METHOD1(Stick, void(bool));
  MOCK_METHOD0(UnStick, void());
  MOCK_METHOD0(Quit, void());

  std::string remote_uri_;
};

struct MockVolumeLauncherIcon : public VolumeLauncherIcon
{
  typedef nux::ObjectPtr<MockVolumeLauncherIcon> Ptr;

  MockVolumeLauncherIcon()
    : VolumeLauncherIcon(Volume::Ptr(volume_ = new MockVolume()),
                         std::make_shared<MockDevicesSettings>())
    , uuid_(std::to_string(g_random_int()))
  {
    ON_CALL(*volume_, GetIdentifier()).WillByDefault(Return(uuid_));
    ON_CALL(*this, Stick(_)).WillByDefault(Invoke(this, &MockVolumeLauncherIcon::ReallyStick));
  }

  void ReallyStick(bool save) { VolumeLauncherIcon::Stick(save); }

  MOCK_METHOD1(Stick, void(bool));
  MOCK_METHOD0(UnStick, void());

  MockVolume* volume_;
  std::string uuid_;
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

    AbstractLauncherIcon::Ptr GetIconByDesktop(std::string const& path) const
    {
      auto const& model = Impl()->model_;
      auto icon = std::find_if(model->begin(), model->end(),
       [&path](AbstractLauncherIcon::Ptr const& i) { return ( i->DesktopFile() == path); });

      if (icon != model->end())
        return *icon;

      return AbstractLauncherIcon::Ptr();
    }

    void ClearModel()
    {
      auto const& model = Impl()->model_;
      std::vector<AbstractLauncherIcon::Ptr> icons;

      for (auto const& icon : *model)
        icons.push_back(icon);

      for (auto const& icon : icons)
        model->RemoveIcon(icon);

      ASSERT_EQ(model->Size(), 0);
    }

    void DisconnectSignals()
    {
      Impl()->view_opened_signal_.Disconnect();
      Impl()->device_section_.icon_added.clear();
      Impl()->model_->icon_removed.clear();
      Impl()->model_->saved.clear();
      Impl()->model_->order_changed.clear();
    }
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
  ASSERT_EQ(lc.Impl()->parent_, &lc);
  ASSERT_TRUE(lc.Impl()->matcher_.IsType(BAMF_TYPE_MATCHER));
  ASSERT_NE(lc.Impl()->model_, nullptr);
  EXPECT_EQ(lc.Impl()->expo_icon_->GetIconType(), AbstractLauncherIcon::IconType::EXPO);
  EXPECT_EQ(lc.Impl()->desktop_icon_->GetIconType(), AbstractLauncherIcon::IconType::DESKTOP);
  EXPECT_GE(lc.Impl()->sort_priority_, AbstractLauncherIcon::DefaultPriority(AbstractLauncherIcon::IconType::APPLICATION));
  EXPECT_EQ(lc.Impl()->model_->GetSublist<BFBLauncherIcon>().size(), 1);
  EXPECT_EQ(lc.Impl()->model_->GetSublist<HudLauncherIcon>().size(), 1);
  EXPECT_EQ(lc.Impl()->model_->GetSublist<TrashLauncherIcon>().size(), 1);
  EXPECT_FALSE(lc.Impl()->launcher_open);
  EXPECT_FALSE(lc.Impl()->launcher_keynav);
  EXPECT_FALSE(lc.Impl()->launcher_grabbed);
  EXPECT_FALSE(lc.Impl()->reactivate_keynav);
  EXPECT_TRUE(lc.Impl()->keynav_restore_window_);
  EXPECT_EQ(lc.Impl()->launcher_key_press_time_, 0);

  for (auto const& fav_uri : favorite_store.GetFavorites())
  {
    auto const& model_icon_it = std::find_if(lc.Impl()->model_->begin(), lc.Impl()->model_->end(),
    [&fav_uri](AbstractLauncherIcon::Ptr const& i) { return (i->RemoteUri() == fav_uri); });

    ASSERT_TRUE((*model_icon_it).IsValid());
  }
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
  const std::string desktop = app::BZR_HANDLE_PATCH;
  MockBamfLauncherIcon::Ptr bamf_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->model_->AddIcon(bamf_icon);

  EXPECT_CALL(*bamf_icon, UnStick());
  EXPECT_CALL(*bamf_icon, Quit()).Times(0);

  favorite_store.favorite_removed.emit(FavoriteStore::URI_PREFIX_APP + desktop);
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
  std::string desktop_file = app::BZR_HANDLE_PATCH;
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

TEST_F(TestLauncherController, CreateFavoriteExpoIcon)
{
  lc.Impl()->expo_icon_->UnStick();
  std::string icon_uri = FavoriteStore::URI_PREFIX_UNITY + "expo-icon";
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);

  ASSERT_TRUE(fav.IsValid());
  EXPECT_EQ(fav, lc.Impl()->expo_icon_);
  EXPECT_EQ(fav->GetIconType(), AbstractLauncherIcon::IconType::EXPO);
  EXPECT_EQ(fav->RemoteUri(), icon_uri);
  EXPECT_TRUE(fav->IsSticky());
  EXPECT_NE(dynamic_cast<ExpoLauncherIcon*>(fav.GetPointer()), nullptr);
}

TEST_F(TestLauncherController, CreateFavoriteDesktopIcon)
{
  lc.Impl()->desktop_icon_->UnStick();
  std::string icon_uri = FavoriteStore::URI_PREFIX_UNITY + "desktop-icon";
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);

  ASSERT_TRUE(fav.IsValid());
  EXPECT_EQ(fav, lc.Impl()->desktop_icon_);
  EXPECT_EQ(fav->GetIconType(), AbstractLauncherIcon::IconType::DESKTOP);
  EXPECT_EQ(fav->RemoteUri(), icon_uri);
  EXPECT_TRUE(fav->IsSticky());
  EXPECT_NE(dynamic_cast<DesktopLauncherIcon*>(fav.GetPointer()), nullptr);
}

TEST_F(TestLauncherController, CreateFavoriteInvalidUnity)
{
  std::string icon_uri = FavoriteStore::URI_PREFIX_UNITY + "foooooo";
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);

  EXPECT_FALSE(fav.IsValid());
}

TEST_F(TestLauncherController, RegisterIconApplication)
{
  AbstractLauncherIcon::Ptr icon(new MockLauncherIcon());
  int pre_priority = icon->SortPriority();

  ASSERT_TRUE(icon->position_saved.empty());
  ASSERT_TRUE(icon->position_forgot.empty());
  ASSERT_TRUE(icon->visibility_changed.empty());
  ASSERT_EQ(lc.Impl()->model_->IconIndex(icon), -1);

  lc.Impl()->RegisterIcon(icon);

  EXPECT_EQ(pre_priority, icon->SortPriority());
  EXPECT_FALSE(icon->position_saved.empty());
  EXPECT_FALSE(icon->position_forgot.empty());
  EXPECT_FALSE(icon->visibility_changed.empty());
  EXPECT_NE(lc.Impl()->model_->IconIndex(icon), -1);
}

TEST_F(TestLauncherController, RegisterIconApplicationWithPriority)
{
  AbstractLauncherIcon::Ptr icon(new MockLauncherIcon());
  lc.Impl()->RegisterIcon(icon, std::numeric_limits<int>::max());
  EXPECT_EQ(icon->SortPriority(), std::numeric_limits<int>::max());
}

TEST_F(TestLauncherController, RegisterIconApplicationWithDefaultPriority)
{
  AbstractLauncherIcon::Ptr icon(new MockLauncherIcon());
  int pre_priority = icon->SortPriority();
  lc.Impl()->RegisterIcon(icon, std::numeric_limits<int>::min());
  EXPECT_EQ(icon->SortPriority(), pre_priority);
}

TEST_F(TestLauncherController, RegisterIconTwoTimesDoesNotWork)
{
  AbstractLauncherIcon::Ptr icon(new MockLauncherIcon());
  lc.Impl()->RegisterIcon(icon, std::numeric_limits<int>::min());
  int pre_registrations = icon->visibility_changed.size();

  lc.Impl()->RegisterIcon(icon, std::numeric_limits<int>::min());
  EXPECT_EQ(icon->visibility_changed.size(), pre_registrations);
}

TEST_F(TestLauncherController, RegisterIconDevice)
{
  AbstractLauncherIcon::Ptr icon(new MockLauncherIcon(AbstractLauncherIcon::IconType::DEVICE));
  int pre_priority = icon->SortPriority();

  lc.Impl()->RegisterIcon(icon);

  EXPECT_EQ(pre_priority, icon->SortPriority());
  EXPECT_FALSE(icon->position_saved.empty());
  EXPECT_FALSE(icon->position_forgot.empty());
  EXPECT_TRUE(icon->visibility_changed.empty());
  EXPECT_NE(lc.Impl()->model_->IconIndex(icon), -1);
}

TEST_F(TestLauncherController, RegisteredIconSavesPosition)
{
  MockBamfLauncherIcon::Ptr app_icon(new MockBamfLauncherIcon(true, "normal-icon.desktop"));
  lc.Impl()->RegisterIcon(app_icon);
  ASSERT_FALSE(favorite_store.IsFavorite(app_icon->RemoteUri()));

  app_icon->Stick(true);
  ASSERT_TRUE(app_icon->IsSticky());
  EXPECT_TRUE(favorite_store.IsFavorite(app_icon->RemoteUri()));
}

TEST_F(TestLauncherController, RegisteredIconForgetsPosition)
{
  auto const& fav = lc.Impl()->GetIconByUri(*(favorite_store.GetFavorites().begin()));
  ASSERT_TRUE(favorite_store.IsFavorite(fav->RemoteUri()));

  fav->UnStick();
  EXPECT_FALSE(favorite_store.IsFavorite(fav->RemoteUri()));
}

TEST_F(TestLauncherController, GetIconByUriDesktop)
{
  lc.Impl()->RegisterIcon(lc.Impl()->desktop_icon_);
  std::string icon_uri = FavoriteStore::URI_PREFIX_UNITY + "desktop-icon";
  auto const& fav = lc.Impl()->GetIconByUri(icon_uri);

  EXPECT_EQ(fav, lc.Impl()->desktop_icon_);
}

TEST_F(TestLauncherController, GetIconByUriExpo)
{
  lc.Impl()->RegisterIcon(lc.Impl()->expo_icon_);
  std::string icon_uri = FavoriteStore::URI_PREFIX_UNITY + "expo-icon";
  auto const& fav = lc.Impl()->GetIconByUri(icon_uri);

  EXPECT_EQ(fav, lc.Impl()->expo_icon_);
}

TEST_F(TestLauncherController, GetIconByUriApplications)
{
  for (auto const& fav_uri : favorite_store.GetFavorites())
  {
    auto const& model_icon_it = std::find_if(lc.Impl()->model_->begin(), lc.Impl()->model_->end(),
    [&fav_uri](AbstractLauncherIcon::Ptr const& i) { return (i->RemoteUri() == fav_uri); });
    ASSERT_NE(model_icon_it, lc.Impl()->model_->end());

    auto const& fav = lc.Impl()->GetIconByUri(fav_uri);
    ASSERT_EQ(fav, *model_icon_it);
  }

  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;
  auto const& fav = lc.Impl()->CreateFavoriteIcon(icon_uri);
  lc.Impl()->RegisterIcon(fav);
  EXPECT_EQ(fav, lc.Impl()->GetIconByUri(icon_uri));
}

TEST_F(TestLauncherController, AddRunningApps)
{
  lc.ClearModel();
  lc.DisconnectSignals();
  lc.Impl()->AddRunningApps();

  std::shared_ptr<GList> apps(bamf_matcher_get_applications(lc.Impl()->matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
      continue;

    BamfApplication* app = BAMF_APPLICATION(l->data);

    ASSERT_NE(g_object_get_qdata(G_OBJECT(app), g_quark_from_static_string("unity-seen")), nullptr);

    auto desktop = bamf_application_get_desktop_file(app);
    std::string path(desktop ? desktop : "");

    if (path.empty())
      continue;

    ASSERT_TRUE(lc.GetIconByDesktop(path).IsValid());
  }
}

TEST_F(TestLauncherController, AddDevices)
{
  lc.ClearModel();
  lc.DisconnectSignals();
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& icons = lc.Impl()->device_section_.GetIcons();
  auto const& device_icon1 = *(icons.begin());
  auto const& device_icon2 = *(std::next(icons.begin()));

  device_icon1->Stick(false);

  lc.Impl()->AddDevices();

  EXPECT_FALSE(lc.Impl()->GetIconByUri(device_icon1->RemoteUri()).IsValid());
  EXPECT_TRUE(lc.Impl()->GetIconByUri(device_icon2->RemoteUri()).IsValid());
}

TEST_F(TestLauncherController, SetupIcons)
{
  lc.ClearModel();
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& model = lc.Impl()->model_;
  int icon_index = 0;

  favorite_store.SetFavorites({ FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE,
                                FavoriteStore::URI_PREFIX_APP + app::SW_CENTER,
                                places::DEVICES_URI,
                                FavoriteStore::URI_PREFIX_APP + app::UPDATE_MANAGER });
  lc.Impl()->SetupIcons();
  lc.DisconnectSignals();

  auto fav = lc.Impl()->GetIconByUri(FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE);
  EXPECT_EQ(model->IconIndex(fav), icon_index);

  fav = lc.Impl()->GetIconByUri(FavoriteStore::URI_PREFIX_APP + app::SW_CENTER);
  EXPECT_EQ(model->IconIndex(fav), ++icon_index);

  for (auto const& device : lc.Impl()->device_section_.GetIcons())
    ASSERT_EQ(model->IconIndex(device), ++icon_index);

  fav = lc.Impl()->GetIconByUri(FavoriteStore::URI_PREFIX_APP + app::UPDATE_MANAGER);
  EXPECT_EQ(model->IconIndex(fav), ++icon_index);

  std::shared_ptr<GList> apps(bamf_matcher_get_applications(lc.Impl()->matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
      continue;

    auto desktop = bamf_application_get_desktop_file(BAMF_APPLICATION(l->data));
    std::string path(desktop ? desktop : "");
    ++icon_index;

    if (path.empty())
      continue;

    auto const& icon = lc.GetIconByDesktop(path);

    ASSERT_TRUE(icon.IsValid());
    ASSERT_EQ(model->IconIndex(icon), icon_index);
  }
}

TEST_F(TestLauncherController, LauncherAddRequestApplicationAdd)
{
  auto const& model = lc.Impl()->model_;
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;
  ASSERT_FALSE(lc.Impl()->GetIconByUri(icon_uri).IsValid());

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& second_app = *(std::next(app_icons.begin()));

  lc.launcher().add_request.emit(icon_uri, second_app);

  auto const& new_icon = lc.Impl()->GetIconByUri(icon_uri);
  ASSERT_TRUE(new_icon.IsValid());
  EXPECT_EQ(model->IconIndex(new_icon), model->IconIndex(second_app) + 1);
}

TEST_F(TestLauncherController, LauncherAddRequestApplicationStick)
{
  auto const& model = lc.Impl()->model_;
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_FILE + desktop;

  MockBamfLauncherIcon::Ptr bamf_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->RegisterIcon(bamf_icon, std::numeric_limits<int>::max());

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());
  ASSERT_LT(model->IconIndex(first_app), model->IconIndex(bamf_icon));

  EXPECT_CALL(*bamf_icon, Stick(false));
  lc.launcher().add_request.emit(icon_uri, first_app);

  EXPECT_EQ(model->IconIndex(bamf_icon), model->IconIndex(first_app) + 1);
}

TEST_F(TestLauncherController, LauncherAddRequestDeviceAdd)
{
  auto const& model = lc.Impl()->model_;
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& icons = lc.Impl()->device_section_.GetIcons();
  auto const& device_icon = *(icons.begin());
  auto const& icon_uri = device_icon->RemoteUri();

  ASSERT_FALSE(lc.Impl()->GetIconByUri(icon_uri).IsValid());

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());

  lc.launcher().add_request.emit(icon_uri, first_app);

  auto const& new_icon = lc.Impl()->GetIconByUri(icon_uri);
  ASSERT_TRUE(new_icon.IsValid());
  EXPECT_EQ(new_icon, device_icon);
  EXPECT_EQ(model->IconIndex(new_icon), model->IconIndex(first_app) + 1);
}

TEST_F(TestLauncherController, LauncherAddRequestDeviceStick)
{
  auto const& model = lc.Impl()->model_;
  MockVolumeLauncherIcon::Ptr device_icon(new MockVolumeLauncherIcon());
  lc.Impl()->RegisterIcon(device_icon, std::numeric_limits<int>::max());

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& second_app = *(std::next(app_icons.begin()));
  ASSERT_LT(model->IconIndex(second_app), model->IconIndex(device_icon));

  EXPECT_CALL(*device_icon, Stick(false));
  lc.launcher().add_request.emit(device_icon->RemoteUri(), second_app);

  EXPECT_EQ(model->IconIndex(device_icon), model->IconIndex(second_app) + 1);
}

TEST_F(TestLauncherController, LauncherRemoveRequestApplicationUnStickAndQuit)
{
  MockBamfLauncherIcon::Ptr bamf_icon(new MockBamfLauncherIcon());

  EXPECT_CALL(*bamf_icon, UnStick());
  EXPECT_CALL(*bamf_icon, Quit());
  lc.launcher().remove_request.emit(bamf_icon);
}

TEST_F(TestLauncherController, LauncherRemoveRequestDeviceEjects)
{
  MockVolumeLauncherIcon::Ptr device_icon(new MockVolumeLauncherIcon());

  EXPECT_CALL(*(device_icon->volume_), CanBeEjected())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*(device_icon->volume_), CanBeStopped())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*(device_icon->volume_), EjectAndShowNotification());
  EXPECT_CALL(*(device_icon->volume_), StopDrive()).Times(0);

  lc.launcher().remove_request.emit(device_icon);
}

TEST_F(TestLauncherController, LauncherRemoveRequestDeviceStops)
{
  MockVolumeLauncherIcon::Ptr device_icon(new MockVolumeLauncherIcon());

  EXPECT_CALL(*(device_icon->volume_), CanBeEjected())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*(device_icon->volume_), CanBeStopped())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*(device_icon->volume_), StopDrive());
  EXPECT_CALL(*(device_icon->volume_), EjectAndShowNotification()).Times(0);

  lc.launcher().remove_request.emit(device_icon);
}

TEST_F(TestLauncherController, LauncherAddRequestSpecial)
{
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;
  ASSERT_FALSE(lc.Impl()->GetIconByUri(icon_uri).IsValid());

  lc.Impl()->OnLauncherAddRequestSpecial(desktop, "", "", 0, 0, 32);

  auto const& sw_center_icon = lc.Impl()->GetIconByUri(icon_uri);
  ASSERT_TRUE(sw_center_icon.IsValid());
  EXPECT_NE(dynamic_cast<SoftwareCenterLauncherIcon*>(sw_center_icon.GetPointer()), nullptr);
}

TEST_F(TestLauncherController, LauncherAddRequestSpecialIgnored)
{
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;

  MockBamfLauncherIcon::Ptr bamf_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->RegisterIcon(bamf_icon, std::numeric_limits<int>::max());
  ASSERT_TRUE(lc.Impl()->GetIconByUri(icon_uri).IsValid());

  EXPECT_CALL(*bamf_icon, Stick(false)).Times(0);

  int previous_model_size = lc.Impl()->model_->Size();
  lc.Impl()->OnLauncherAddRequestSpecial(desktop, "", "", 0, 0, 32);

  EXPECT_EQ(previous_model_size, lc.Impl()->model_->Size());
}

TEST_F(TestLauncherController, SaveIconsOrder)
{
  favorite_store.ClearFavorites();
  lc.ClearModel();
  lc.DisconnectSignals();
  int priority = 0;

  MockBamfLauncherIcon::Ptr sticky_app(new MockBamfLauncherIcon(true, "sticky-app"));
  sticky_app->Stick(false);
  lc.Impl()->RegisterIcon(sticky_app, ++priority);

  MockBamfLauncherIcon::Ptr invisible_app(new MockBamfLauncherIcon(true, "invisible-app"));
  invisible_app->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
  lc.Impl()->RegisterIcon(invisible_app, ++priority);

  MockVolumeLauncherIcon::Ptr sticky_device(new MockVolumeLauncherIcon());
  sticky_device->Stick(false);
  lc.Impl()->RegisterIcon(sticky_device, ++priority);

  MockVolumeLauncherIcon::Ptr device(new MockVolumeLauncherIcon());
  lc.Impl()->RegisterIcon(device, ++priority);

  MockBamfLauncherIcon::Ptr running_app(new MockBamfLauncherIcon(true, "running-app"));
  lc.Impl()->RegisterIcon(running_app, ++priority);

  lc.Impl()->SaveIconsOrder();

  auto it = favorite_store.GetFavorites().begin();

  ASSERT_EQ(*it, sticky_app->RemoteUri()); ++it;
  ASSERT_EQ(*it, sticky_device->RemoteUri()); ++it;
  ASSERT_EQ(*it, places::DEVICES_URI); ++it;
  ASSERT_EQ(*it, places::APPS_URI); ++it;
  ASSERT_EQ(it, favorite_store.GetFavorites().end());
}

TEST_F(TestLauncherController, SaveIconsOrderWithOnlyStickyIcons)
{
  favorite_store.ClearFavorites();
  lc.ClearModel();
  int priority = 0;

  MockBamfLauncherIcon::Ptr sticky_app(new MockBamfLauncherIcon(true, "sticky-app"));
  sticky_app->Stick(false);
  lc.Impl()->RegisterIcon(sticky_app, ++priority);

  MockVolumeLauncherIcon::Ptr sticky_device(new MockVolumeLauncherIcon());
  sticky_device->Stick(false);
  lc.Impl()->RegisterIcon(sticky_device, ++priority);

  lc.Impl()->SaveIconsOrder();

  auto it = favorite_store.GetFavorites().begin();

  ASSERT_EQ(*it, sticky_app->RemoteUri()); ++it;
  ASSERT_EQ(*it, sticky_device->RemoteUri()); ++it;
  ASSERT_EQ(*it, places::APPS_URI); ++it;
  ASSERT_EQ(*it, places::DEVICES_URI); ++it;
  ASSERT_EQ(it, favorite_store.GetFavorites().end());
}

TEST_F(TestLauncherController, SaveIconsOrderTriesToKeepIconProvidersOrder)
{
  favorite_store.ClearFavorites();
  lc.ClearModel();
  int priority = 0;

  favorite_store.SetFavorites({FavoriteStore::URI_PREFIX_APP + "foo.desktop", places::DEVICES_URI,
                               FavoriteStore::URI_PREFIX_APP + "bar.desktop", places::APPS_URI,
                               FavoriteStore::URI_PREFIX_APP + "foobar.desktop"});

  MockBamfLauncherIcon::Ptr sticky_app(new MockBamfLauncherIcon(true, "sticky-app"));
  sticky_app->Stick(false);
  lc.Impl()->RegisterIcon(sticky_app, ++priority);

  MockVolumeLauncherIcon::Ptr sticky_device(new MockVolumeLauncherIcon());
  sticky_device->Stick(false);
  lc.Impl()->RegisterIcon(sticky_device, ++priority);

  lc.Impl()->SaveIconsOrder();

  auto it = favorite_store.GetFavorites().begin();

  ASSERT_EQ(*it, places::DEVICES_URI); ++it;
  ASSERT_EQ(*it, places::APPS_URI); ++it;
  ASSERT_EQ(*it, sticky_app->RemoteUri()); ++it;
  ASSERT_EQ(*it, sticky_device->RemoteUri()); ++it;
  ASSERT_EQ(it, favorite_store.GetFavorites().end());
}

TEST_F(TestLauncherController, SaveIconsOrderTriesToKeepIconProvidersOrder2)
{
  favorite_store.ClearFavorites();
  lc.ClearModel();
  int priority = 0;

  MockBamfLauncherIcon::Ptr sticky_app(new MockBamfLauncherIcon(true, "sticky-app"));
  sticky_app->Stick(false);
  lc.Impl()->RegisterIcon(sticky_app, ++priority);

  MockVolumeLauncherIcon::Ptr sticky_device(new MockVolumeLauncherIcon());
  sticky_device->Stick(false);
  lc.Impl()->RegisterIcon(sticky_device, ++priority);

  favorite_store.SetFavorites({places::DEVICES_URI, sticky_app->RemoteUri(), places::APPS_URI});
  lc.Impl()->SaveIconsOrder();

  auto it = favorite_store.GetFavorites().begin();

  ASSERT_EQ(*it, places::DEVICES_URI); ++it;
  ASSERT_EQ(*it, sticky_app->RemoteUri()); ++it;
  ASSERT_EQ(*it, places::APPS_URI); ++it;
  ASSERT_EQ(*it, sticky_device->RemoteUri()); ++it;
  ASSERT_EQ(it, favorite_store.GetFavorites().end());
}

TEST_F(TestLauncherController, SortAndUpdate)
{
  lc.ClearModel();

  MockVolumeLauncherIcon::Ptr device(new MockVolumeLauncherIcon());
  lc.Impl()->RegisterIcon(device, 0);

  for (int i = 0; i < 15; ++i)
  {
    MockBamfLauncherIcon::Ptr app(new MockBamfLauncherIcon());
    app->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, (i % 5) == 0);
    lc.Impl()->RegisterIcon(app, 0);
  }

  int expected_shortcut = 1;

  for (auto const& icon : *(lc.Impl()->model_))
  {
    if (icon->IsVisible() && icon->GetIconType() == AbstractLauncherIcon::IconType::APPLICATION)
    {
      ASSERT_EQ(icon->GetShortcut(), std::to_string(expected_shortcut % 10)[0]);
      ++expected_shortcut;
    }
    else
    {
      ASSERT_EQ(icon->GetShortcut(), 0);
    }
  }
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedNew)
{
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + app::BZR_HANDLE_PATCH;

  favorite_store.favorite_added.emit(icon_uri, "", true);

  auto const& new_icon = lc.Impl()->GetIconByUri(icon_uri);
  ASSERT_TRUE(new_icon.IsValid());
  EXPECT_TRUE(new_icon->IsSticky());
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedNewBeforeIcon)
{
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + app::BZR_HANDLE_PATCH;
  auto const& model = lc.Impl()->model_;

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());
  favorite_store.favorite_added.emit(icon_uri, first_app->RemoteUri(), true);

  auto const& new_icon = lc.Impl()->GetIconByUri(icon_uri);

  ASSERT_TRUE(new_icon.IsValid());
  EXPECT_TRUE(new_icon->IsSticky());
  EXPECT_EQ(model->IconIndex(new_icon), model->IconIndex(first_app) - 1);
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedNewAfterIcon)
{
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + app::BZR_HANDLE_PATCH;
  auto const& model = lc.Impl()->model_;

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());
  favorite_store.favorite_added.emit(icon_uri, first_app->RemoteUri(), false);

  auto const& new_icon = lc.Impl()->GetIconByUri(icon_uri);

  ASSERT_TRUE(new_icon.IsValid());
  EXPECT_TRUE(new_icon->IsSticky());
  EXPECT_EQ(model->IconIndex(new_icon), model->IconIndex(first_app) + 1);
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedStick)
{
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;

  MockBamfLauncherIcon::Ptr app_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->RegisterIcon(app_icon, std::numeric_limits<int>::max());

  EXPECT_CALL(*app_icon, Stick(false));
  favorite_store.favorite_added.emit(icon_uri, "", false);
  EXPECT_TRUE(app_icon->IsSticky());
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedStickBefore)
{
  auto const& model = lc.Impl()->model_;
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;

  MockBamfLauncherIcon::Ptr app_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->RegisterIcon(app_icon, std::numeric_limits<int>::max());

  auto app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());
  ASSERT_LT(model->IconIndex(first_app), model->IconIndex(app_icon));

  EXPECT_CALL(*app_icon, Stick(false));

  favorite_store.favorite_added.emit(icon_uri, first_app->RemoteUri(), false);
  EXPECT_TRUE(app_icon->IsSticky());
  EXPECT_EQ(model->IconIndex(app_icon), model->IconIndex(first_app) + 1);
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteAddedStickAfter)
{
  auto const& model = lc.Impl()->model_;
  std::string desktop = app::BZR_HANDLE_PATCH;
  std::string icon_uri = FavoriteStore::URI_PREFIX_APP + desktop;

  MockBamfLauncherIcon::Ptr app_icon(new MockBamfLauncherIcon(desktop));
  lc.Impl()->RegisterIcon(app_icon, std::numeric_limits<int>::max());

  auto const& app_icons = model->GetSublist<BamfLauncherIcon>();
  auto const& first_app = *(app_icons.begin());
  ASSERT_LT(model->IconIndex(first_app), model->IconIndex(app_icon));

  EXPECT_CALL(*app_icon, Stick(false));

  favorite_store.favorite_added.emit(icon_uri, first_app->RemoteUri(), true);
  EXPECT_TRUE(app_icon->IsSticky());
  EXPECT_EQ(model->IconIndex(app_icon), model->IconIndex(first_app) - 1);
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteRemovedApplication)
{
  MockBamfLauncherIcon::Ptr app_icon(new MockBamfLauncherIcon(true, "sticky-icon"));
  lc.Impl()->RegisterIcon(app_icon);
  app_icon->Stick(false);

  EXPECT_CALL(*app_icon, UnStick());
  favorite_store.favorite_removed.emit(app_icon->RemoteUri());
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteRemovedDevice)
{
  lc.ClearModel();
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& model = lc.Impl()->model_;

  auto const& icons = lc.Impl()->device_section_.GetIcons();
  auto const& device_icon(*(icons.begin()));

  favorite_store.SetFavorites({ FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE,
                                device_icon->RemoteUri(),
                                FavoriteStore::URI_PREFIX_APP + app::UPDATE_MANAGER });
  lc.Impl()->SetupIcons();
  lc.DisconnectSignals();

  ASSERT_EQ(model->IconIndex(device_icon), 1);

  favorite_store.RemoveFavorite(device_icon->RemoteUri());
  favorite_store.favorite_removed.emit(device_icon->RemoteUri());

  auto const& app_icons = lc.Impl()->model_->GetSublist<BamfLauncherIcon>();
  auto const& last_app = *(app_icons.rbegin());
  EXPECT_EQ(model->IconIndex(device_icon), model->IconIndex(last_app) + 1);
}

TEST_F(TestLauncherController, OnFavoriteStoreFavoriteRemovedDeviceSection)
{
  lc.ClearModel();
  lc.Impl()->device_section_ = MockDeviceLauncherSection();
  auto const& model = lc.Impl()->model_;

  auto const& icons = lc.Impl()->device_section_.GetIcons();
  auto const& device_icon1(*(icons.begin()));
  auto const& device_icon2(*(std::next(icons.begin())));

  favorite_store.SetFavorites({ places::DEVICES_URI,
                                FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE });
  lc.Impl()->SetupIcons();
  lc.DisconnectSignals();

  ASSERT_EQ(model->IconIndex(device_icon1), 0);
  ASSERT_EQ(model->IconIndex(device_icon2), 1);

  favorite_store.RemoveFavorite(places::DEVICES_URI);
  favorite_store.favorite_removed.emit(places::DEVICES_URI);

  auto const& app_icons = lc.Impl()->model_->GetSublist<BamfLauncherIcon>();
  auto const& last_app = *(app_icons.rbegin());
  EXPECT_EQ(model->IconIndex(device_icon1), model->IconIndex(last_app) + 1);
  EXPECT_EQ(model->IconIndex(device_icon2), model->IconIndex(last_app) + 2);
}

TEST_F(TestLauncherController, OnViewOpened)
{
  auto const& app_icons = lc.Impl()->model_->GetSublist<BamfLauncherIcon>();
  auto const& last_app = *(app_icons.rbegin());

  auto app = bamf_matcher_get_application_for_desktop_file(lc.Impl()->matcher_, app::BZR_HANDLE_PATCH.c_str(), TRUE);
  g_signal_emit_by_name(lc.Impl()->matcher_, "view-opened", app);
  lc.DisconnectSignals();

  auto const& icon = lc.GetIconByDesktop(app::BZR_HANDLE_PATCH);
  ASSERT_TRUE(icon.IsValid());

  ASSERT_EQ(lc.Impl()->model_->IconIndex(icon), lc.Impl()->model_->IconIndex(last_app) + 1);
}

}
