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
 */

#include <gmock/gmock.h>

// #include "FavoriteStore.h"
// #include "LauncherController.h"
// #include "LauncherControllerPrivate.h"
// #include "ExpoLauncherIcon.h"
// #include "DesktopLauncherIcon.h"
// #include "MockLauncherIcon.h"
// #include "BFBLauncherIcon.h"
// #include "HudLauncherIcon.h"
// #include "TrashLauncherIcon.h"
// #include "VolumeLauncherIcon.h"
// #include "SoftwareCenterLauncherIcon.h"
#include <Nux/Nux.h>
#include "PanelMenuView.h"
#include "PanelStyle.h"
#include "UnitySettings.h"
#include "UBusMessages.h"
#include "StandaloneWindowManager.h"
#include "test_utils.h"
// #include "test_uscreen_mock.h"
// #include "test_mock_devices.h"
// #include "bamf-mock-application.h"

// using namespace unity::launcher;
using namespace testing;

namespace unity
{

// namespace
// {
// namespace places
// {
// const std::string APPS_URI = "unity://running-apps";
// const std::string DEVICES_URI = "unity://devices";
// }
// 
// namespace app
// {
//  const std::string UBUNTU_ONE = BUILDDIR "/tests/data/applications/ubuntuone-installer.desktop";
//  const std::string SW_CENTER = BUILDDIR "/tests/data/applications/ubuntu-software-center.desktop";
//  const std::string UPDATE_MANAGER = BUILDDIR "/tests/data/applications/update-manager.desktop";
//  const std::string BZR_HANDLE_PATCH = BUILDDIR "/tests/data/applications/bzr-handle-patch.desktop";
//  const std::string NO_ICON = BUILDDIR "/tests/data/applications/no-icon.desktop";
// }
// }

// struct MockFavoriteStore : FavoriteStore
// {
//   MockFavoriteStore()
//   {
//     fav_list_ = { FavoriteStore::URI_PREFIX_APP + app::UBUNTU_ONE,
//                   FavoriteStore::URI_PREFIX_APP + app::SW_CENTER,
//                   FavoriteStore::URI_PREFIX_APP + app::UPDATE_MANAGER };
//   }
// 
//   FavoriteList const& GetFavorites() const
//   {
//     return fav_list_;
//   }
// 
//   void AddFavorite(std::string const& icon_uri, int position)
//   {
//     if (!IsValidFavoriteUri(icon_uri))
//       return;
// 
//     if (position < 0)
//       fav_list_.push_back(icon_uri);
//     else
//       fav_list_.insert(std::next(fav_list_.begin(), position), icon_uri);
//   }
// 
//   void RemoveFavorite(std::string const& icon_uri)
//   {
//     fav_list_.remove(icon_uri);
//   }
// 
//   void MoveFavorite(std::string const& icon_uri, int position)
//   {
//     RemoveFavorite(icon_uri);
//     AddFavorite(icon_uri, position);
//   }
// 
//   bool IsFavorite(std::string const& icon_uri) const
//   {
//     return std::find(fav_list_.begin(), fav_list_.end(), icon_uri) != fav_list_.end();
//   }
// 
//   int FavoritePosition(std::string const& icon_uri) const
//   {
//     auto it = std::find(fav_list_.begin(), fav_list_.end(), icon_uri);
// 
//     if (it != fav_list_.end())
//       return std::distance(fav_list_.begin(), it);
// 
//     return -1;
//   }
// 
//   void SetFavorites(FavoriteList const& icon_uris)
//   {
//     fav_list_ = icon_uris;
//   }
// 
//   void ClearFavorites()
//   {
//     fav_list_.clear();
//   }
// 
// private:
//   FavoriteList fav_list_;
// };
// 
// struct MockApplicationLauncherIcon : public ApplicationLauncherIcon
// {
//   typedef nux::ObjectPtr<MockApplicationLauncherIcon> Ptr;
//   typedef bool Fake;
// 
//   MockApplicationLauncherIcon(Fake = true, std::string const& remote_uri = "")
//     : ApplicationLauncherIcon(BAMF_APPLICATION(bamf_mock_application_new()))
//     , remote_uri_(remote_uri)
//   {
//     InitMock();
//     SetQuirk(Quirk::VISIBLE, true);
//   }
// 
//   explicit MockApplicationLauncherIcon(BamfApplication* app)
//     : ApplicationLauncherIcon(app)
//   {
//     InitMock();
//   }
// 
//   MockApplicationLauncherIcon(std::string const& desktop_file)
//     : ApplicationLauncherIcon(bamf_matcher_get_application_for_desktop_file(bamf_matcher_get_default(), desktop_file.c_str(), TRUE))
//   {
//     InitMock();
//   }
// 
//   void InitMock()
//   {
//     ON_CALL(*this, Stick(_)).WillByDefault(Invoke(this, &MockApplicationLauncherIcon::ReallyStick));
//   }
// 
//   std::string GetRemoteUri()
//   {
//     if (remote_uri_.empty())
//       return ApplicationLauncherIcon::GetRemoteUri();
//     else
//       return FavoriteStore::URI_PREFIX_APP + remote_uri_;
//   }
// 
//   void ReallyStick(bool save) { ApplicationLauncherIcon::Stick(save); }
// 
//   MOCK_METHOD1(Stick, void(bool));
//   MOCK_METHOD0(UnStick, void());
//   MOCK_METHOD0(Quit, void());
// 
//   std::string remote_uri_;
// };
// 
// struct MockVolumeLauncherIcon : public VolumeLauncherIcon
// {
//   typedef nux::ObjectPtr<MockVolumeLauncherIcon> Ptr;
// 
//   MockVolumeLauncherIcon()
//     : VolumeLauncherIcon(Volume::Ptr(volume_ = new MockVolume()),
//                          std::make_shared<MockDevicesSettings>())
//     , uuid_(std::to_string(g_random_int()))
//   {
//     ON_CALL(*volume_, GetIdentifier()).WillByDefault(Return(uuid_));
//     ON_CALL(*this, Stick(_)).WillByDefault(Invoke(this, &MockVolumeLauncherIcon::ReallyStick));
//   }
// 
//   void ReallyStick(bool save) { VolumeLauncherIcon::Stick(save); }
// 
//   MOCK_METHOD1(Stick, void(bool));
//   MOCK_METHOD0(UnStick, void());
// 
//   MockVolume* volume_;
//   std::string uuid_;
// };

// namespace launcher
// {

class MockPanelMenuView : public PanelMenuView
{
  public:
    using PanelMenuView::_panel_title;
    using PanelMenuView::RefreshTitle;

  protected:
    virtual std::string GetActiveViewName(bool use_appname) const
    {
      return "<>'";
    }
};

struct TestPanelMenuView : public testing::Test
{
  virtual void SetUp()
  {
//     lc.multiple_launchers = true;
  }

  void ProcessUBusMessages()
  {
    bool expired = false;
    glib::Idle idle([&] { expired = true; return false; },
                    glib::Source::Priority::LOW);
    Utils::WaitUntil(expired);
  }

protected:

  // The order is important, i.e. PanelMenuView needs
  // panel::Style that needs Settings
  Settings settings;
  panel::Style panelStyle;
  MockPanelMenuView panelMenuView;
};
// }

TEST_F(TestPanelMenuView, Escaping)
{
  static const char *escapedText = "Panel d&amp;Inici";
  EXPECT_EQ(panelMenuView._panel_title, "");

  UBusManager ubus;
  ubus.SendMessage(UBUS_LAUNCHER_START_KEY_NAV, NULL);
  ubus.SendMessage(UBUS_LAUNCHER_SELECTION_CHANGED,
                   g_variant_new_string(escapedText));
  ProcessUBusMessages();

  ubus.SendMessage(UBUS_LAUNCHER_END_KEY_NAV, NULL);
  ProcessUBusMessages();

  StandaloneWindowManager *wm = dynamic_cast<StandaloneWindowManager *>(&WindowManager::Default());
  EXPECT_TRUE(wm != nullptr);
  // Change the wm to trick the panel view to call GetActiveViewName
  wm->SetScaleActive(true);
  wm->SetScaleActiveForGroup(true);
  panelMenuView.RefreshTitle();

  EXPECT_EQ(panelMenuView._panel_title, "&lt;&gt;&apos;");
}

}
