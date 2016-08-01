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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gmock/gmock.h>
using namespace testing;


#include "DevicesSettings.h"
#include "VolumeLauncherIcon.h"
#include "FavoriteStore.h"
#include "test_utils.h"
#include "test_mock_devices.h"
#include "test_mock_filemanager.h"
#include "mock-application.h"

using namespace unity;
using namespace unity::launcher;
using namespace testmocks;

namespace
{

struct TestVolumeLauncherIcon : public Test
{
  TestVolumeLauncherIcon()
    : volume_(std::make_shared<MockVolume::Nice>())
    , settings_(std::make_shared<MockDevicesSettings::Nice>())
    , notifications_(std::make_shared<MockDeviceNotificationDisplay::Nice>())
    , file_manager_(std::make_shared<MockFileManager::Nice>())
  {
    SetupVolumeDefaultBehavior();
    SetupSettingsDefaultBehavior();
    icon_ = new VolumeLauncherIcon(volume_, settings_, notifications_, file_manager_);
  }

  void SetupSettingsDefaultBehavior()
  {
    ON_CALL(*settings_, IsABlacklistedDevice(_)).WillByDefault(Return(false));
  }

  void SetupVolumeDefaultBehavior()
  {
    ON_CALL(*volume_, CanBeFormatted()).WillByDefault(Return(false));
    ON_CALL(*volume_, CanBeRemoved()).WillByDefault(Return(false));
    ON_CALL(*volume_, CanBeStopped()).WillByDefault(Return(false));
    ON_CALL(*volume_, GetName()).WillByDefault(Return("Test Name"));
    ON_CALL(*volume_, GetIconName()).WillByDefault(Return("Test Icon Name"));
    ON_CALL(*volume_, GetIdentifier()).WillByDefault(Return("Test Identifier"));
    ON_CALL(*volume_, GetUnixDevicePath()).WillByDefault(Return("/dev/sda1"));
    ON_CALL(*volume_, GetUri()).WillByDefault(Return("file:///media/user/device_uri"));
    ON_CALL(*volume_, HasSiblings()).WillByDefault(Return(false));
    ON_CALL(*volume_, CanBeEjected()).WillByDefault(Return(false));
    ON_CALL(*volume_, IsMounted()).WillByDefault(Return(true));
    ON_CALL(*volume_, Mount()).WillByDefault(Invoke([this] { volume_->mounted.emit(); }));
    ON_CALL(*volume_, Unmount()).WillByDefault(Invoke([this] { volume_->unmounted.emit(); }));
    ON_CALL(*volume_, Eject()).WillByDefault(Invoke([this] { volume_->ejected.emit(); }));
    ON_CALL(*volume_, StopDrive()).WillByDefault(Invoke([this] { volume_->stopped.emit(); }));
  }

  glib::Object<DbusmenuMenuitem> GetMenuItemAtIndex(int index)
  {
    auto menuitems = icon_->GetMenus();
    auto menuitem = menuitems.begin();
    std::advance(menuitem, index);

    return *menuitem;
  }

  MockVolume::Ptr volume_;
  MockDevicesSettings::Ptr settings_;
  MockDeviceNotificationDisplay::Ptr notifications_;
  MockFileManager::Ptr file_manager_;
  VolumeLauncherIcon::Ptr icon_;
  std::string old_lang_;
};

struct TestVolumeLauncherIconDelayedConstruction : TestVolumeLauncherIcon
{
  TestVolumeLauncherIconDelayedConstruction()
  {
    icon_ = nullptr;
  }

  void CreateIcon()
  {
    icon_ = new VolumeLauncherIcon(volume_, settings_, notifications_, file_manager_);
  }
};

TEST_F(TestVolumeLauncherIcon, TestIconType)
{
  EXPECT_EQ(icon_->GetIconType(), AbstractLauncherIcon::IconType::DEVICE);
}

TEST_F(TestVolumeLauncherIconDelayedConstruction, TestRunningOnClosed)
{
  ON_CALL(*file_manager_, WindowsForLocation(_)).WillByDefault(Return(WindowList()));
  CreateIcon();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIconDelayedConstruction, TestRunningOnOpened)
{
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win})));
  CreateIcon();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIcon, FilemanagerSignalDisconnection)
{
  ASSERT_FALSE(file_manager_->locations_changed.empty());
  icon_ = nullptr;
  EXPECT_TRUE(file_manager_->locations_changed.empty());
}

TEST_F(TestVolumeLauncherIcon, TestRunningStateOnLocationChangedClosed)
{
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList()));
  file_manager_->locations_changed.emit();
  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIcon, TestRunningStateOnLocationChangedOpened)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win1, win2})));
  file_manager_->locations_changed.emit();
  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIcon, RunningState)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win1, win2})));
  file_manager_->locations_changed.emit();
  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList()));
  file_manager_->locations_changed.emit();
  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIcon, ActiveState)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win1, win2})));
  file_manager_->locations_changed.emit();
  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));

  win2->LocalFocus();
  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList()));
  file_manager_->locations_changed.emit();
  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));
}

TEST_F(TestVolumeLauncherIcon, WindowsCount)
{
  WindowList windows((g_random_int() % 10) + 5);
  for (unsigned i = 0; i < windows.capacity(); ++i)
    windows[i] = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(windows));
  file_manager_->locations_changed.emit();
  EXPECT_EQ(icon_->Windows().size(), windows.size());
}

TEST_F(TestVolumeLauncherIcon, WindowsPerMonitor)
{
  WindowList windows((g_random_int() % 10) + 5);
  for (unsigned i = 0; i < windows.capacity(); ++i)
  {
    auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
    win->monitor_ = i % 2;
    windows[i] = win;
  }

  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(windows));
  file_manager_->locations_changed.emit();

  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(0), (windows.size() / 2) + (windows.size() % 2));
  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(1), windows.size() / 2);
}

TEST_F(TestVolumeLauncherIcon, WindowsOnMonitorChanges)
{
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win})));
  file_manager_->locations_changed.emit();

  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(0), 1);
  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(1), 0);

  win->SetMonitor(1);
  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(0), 0);
  EXPECT_EQ(icon_->WindowsVisibleOnMonitor(1), 1);
}

TEST_F(TestVolumeLauncherIcon, TestPosition)
{
  EXPECT_EQ(icon_->position(), AbstractLauncherIcon::Position::FLOATING);
}

TEST_F(TestVolumeLauncherIcon, TestTooltipText)
{
  EXPECT_EQ(volume_->GetName(), icon_->tooltip_text());
}

TEST_F(TestVolumeLauncherIcon, TestIconName)
{
  EXPECT_EQ(volume_->GetIconName(), icon_->icon_name());
}

TEST_F(TestVolumeLauncherIcon, TestVisibility_InitiallyMountedVolume)
{
  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, RemoteUri)
{
  EXPECT_EQ(icon_->GetRemoteUri(), FavoriteStore::URI_PREFIX_DEVICE + volume_->GetIdentifier());
}

TEST_F(TestVolumeLauncherIconDelayedConstruction, TestVisibility_InitiallyMountedBlacklistedVolume)
{
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  CreateIcon();

  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}


TEST_F(TestVolumeLauncherIconDelayedConstruction, TestVisibility_InitiallyUnmountedVolume)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  CreateIcon();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}


TEST_F(TestVolumeLauncherIconDelayedConstruction, TestVisibility_InitiallyUnmountedBlacklistedVolume)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  CreateIcon();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestSettingsChangedSignal)
{
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));
  settings_->changed.emit();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibilityAfterUnmount)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);

  volume_->changed.emit();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIconDelayedConstruction, TestVisibilityAfterUnmount_BlacklistedVolume)
{
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  CreateIcon();

  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(0);

  volume_->changed.emit();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibilityWithWindows)
{
  ON_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillByDefault(Return(false));
  settings_->changed.emit();
  ASSERT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win})));
  file_manager_->locations_changed.emit();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibilityWithWindows_Blacklisted)
{
  ON_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillByDefault(Return(true));
  settings_->changed.emit();
  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*file_manager_, WindowsForLocation(volume_->GetUri())).WillByDefault(Return(WindowList({win})));
  file_manager_->locations_changed.emit();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_VolumeWithoutIdentifier)
{
  EXPECT_CALL(*volume_, GetIdentifier())
    .WillRepeatedly(Return(""));

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_Success)
{
  ON_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillByDefault(Return(false));
  settings_->changed.emit();
  ASSERT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
  auto menuitem = GetMenuItemAtIndex(4);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToBlacklist(volume_->GetIdentifier())).Times(1);
  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillRepeatedly(Return(true));
  settings_->changed.emit(); // TryToBlacklist() works if DevicesSettings emits a changed signal.

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_Failure)
{
  auto menuitem = GetMenuItemAtIndex(4);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToBlacklist(volume_->GetIdentifier())).Times(1);
  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestLockToLauncherMenuItem_Success)
{
  ON_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillByDefault(Return(true));
  settings_->changed.emit();
  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));

  auto menuitem = GetMenuItemAtIndex(4);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Lock to Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToUnblacklist(volume_->GetIdentifier())).Times(1);
  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_CALL(*settings_, IsABlacklistedDevice(_)).WillRepeatedly(Return(false));
  settings_->changed.emit(); // TryToBlacklist() works if DevicesSettings emits a changed signal.

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestLockToLauncherMenuItem_Failure)
{
  ON_CALL(*settings_, IsABlacklistedDevice(volume_->GetIdentifier())).WillByDefault(Return(true));
  settings_->changed.emit();
  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));

  auto menuitem = GetMenuItemAtIndex(4);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Lock to Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToUnblacklist(volume_->GetIdentifier())).Times(1);
  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestOpenMenuItem)
{
  auto menuitem = GetMenuItemAtIndex(0);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Open");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  ON_CALL(*volume_, IsMounted()).WillByDefault(Return(false));
  uint64_t time = g_random_int();

  InSequence seq;
  EXPECT_CALL(*volume_, Mount());
  EXPECT_CALL(*file_manager_, Open(volume_->GetUri(), time));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
}

TEST_F(TestVolumeLauncherIcon, TestNameMenuItem)
{
  auto menuitem = GetMenuItemAtIndex(2);

  EXPECT_EQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "<b>" + volume_->GetName() + "</b>");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, QuicklistMenuItem::MARKUP_ENABLED_PROPERTY));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY));

  uint64_t time = g_random_int();
  ON_CALL(*volume_, IsMounted()).WillByDefault(Return(false));

  InSequence seq;
  EXPECT_CALL(*volume_, Mount());
  EXPECT_CALL(*file_manager_, Open(volume_->GetUri(), time));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem_NotEjectableVolume)
{
  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Eject");
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));

  auto menuitem = GetMenuItemAtIndex(5);

  EXPECT_CALL(*volume_, Eject());
  EXPECT_CALL(*notifications_, Display(volume_->GetName()));

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Eject");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem_NotStoppableVolume)
{
  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Safely remove");
}

TEST_F(TestVolumeLauncherIcon, TestSafelyRemoveMenuItem)
{
  EXPECT_CALL(*volume_, CanBeStopped())
    .WillRepeatedly(Return(true));

  auto menuitem = GetMenuItemAtIndex(5);

  EXPECT_CALL(*volume_, StopDrive())
    .Times(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Safely remove");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_UnmountedVolume)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}


TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_EjectableVolume)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_StoppableVolume)
{
  EXPECT_CALL(*volume_, CanBeStopped())
    .WillRepeatedly(Return(true));

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem)
{
  auto menuitem = GetMenuItemAtIndex(5);

  EXPECT_CALL(*volume_, Unmount())
    .Times(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
}

TEST_F(TestVolumeLauncherIcon, TestCanBeEject)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));
  ASSERT_TRUE(icon_->CanEject());

  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(false));
  ASSERT_FALSE(icon_->CanEject());

}

TEST_F(TestVolumeLauncherIcon, TestEject)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));

  EXPECT_CALL(*volume_, Eject());
  EXPECT_CALL(*notifications_, Display(volume_->GetName()));
  icon_->EjectAndShowNotification();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved)
{
  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(1);

  volume_->removed.emit();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved_RemovabledVolume)
{
  EXPECT_CALL(*volume_, CanBeRemoved())
    .WillRepeatedly(Return(true));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(1);

  volume_->removed.emit();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved_RemovableAndBlacklistedVolume)
{
  EXPECT_CALL(*volume_, CanBeRemoved())
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(1);

  volume_->removed.emit();
}

TEST_F(TestVolumeLauncherIcon, Stick)
{
  bool saved = false;
  icon_->position_saved.connect([&saved] {saved = true;});

  EXPECT_CALL(*settings_, TryToUnblacklist(volume_->GetIdentifier()));
  icon_->Stick(false);
  EXPECT_TRUE(icon_->IsSticky());
  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_FALSE(saved);
}

TEST_F(TestVolumeLauncherIcon, StickAndSave)
{
  bool saved = false;
  icon_->position_saved.connect([&saved] {saved = true;});

  EXPECT_CALL(*settings_, TryToUnblacklist(volume_->GetIdentifier()));
  icon_->Stick(true);
  EXPECT_TRUE(icon_->IsSticky());
  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(saved);
}

TEST_F(TestVolumeLauncherIcon, Unstick)
{
  bool forgot = false;
  icon_->position_forgot.connect([&forgot] {forgot = true;});

  EXPECT_CALL(*settings_, TryToUnblacklist(_));
  icon_->Stick(false);
  ASSERT_TRUE(icon_->IsSticky());
  ASSERT_TRUE(icon_->IsVisible());

  icon_->UnStick();
  EXPECT_FALSE(icon_->IsSticky());
  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(forgot);
}

TEST_F(TestVolumeLauncherIcon, ActivateMounted)
{
  uint64_t time = g_random_int();
  InSequence seq;
  EXPECT_CALL(*volume_, Mount()).Times(0);
  EXPECT_CALL(*file_manager_, Open(volume_->GetUri(), time));
  icon_->Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, time));
}

TEST_F(TestVolumeLauncherIcon, ActivateUnmounted)
{
  uint64_t time = g_random_int();
  ON_CALL(*volume_, IsMounted()).WillByDefault(Return(false));
  InSequence seq;
  EXPECT_CALL(*volume_, Mount());
  EXPECT_CALL(*file_manager_, Open(volume_->GetUri(), time));
  icon_->Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, time));
}

TEST_F(TestVolumeLauncherIcon, ShouldHighlightOnDragFilesValid)
{
  DndData data;
  std::string data_string = "file://file1\ndir2/file2\nfile://file3\nfile://dirN/fileN";
  data.Fill(data_string.c_str());
  EXPECT_TRUE(icon_->ShouldHighlightOnDrag(data));
}

TEST_F(TestVolumeLauncherIcon, ShouldHighlightOnDragFilesInvalid)
{
  DndData data;
  std::string data_string = "file1\ndir2/file2\napplication://file3\nunity://lens";
  data.Fill(data_string.c_str());
  EXPECT_FALSE(icon_->ShouldHighlightOnDrag(data));
}

TEST_F(TestVolumeLauncherIcon, QueryAcceptDrop)
{
  DndData data;
  EXPECT_EQ(nux::DNDACTION_NONE, icon_->QueryAcceptDrop(data));

  std::string data_string = "file://foo/file";
  data.Fill(data_string.c_str());
  EXPECT_EQ(nux::DNDACTION_COPY, icon_->QueryAcceptDrop(data));
}

TEST_F(TestVolumeLauncherIcon, AcceptDropUnmounted)
{
  DndData data;
  std::string data_string = "file://file1\ndir2/file2\nfile://file3\nfile://dirN/fileN";
  data.Fill(data_string.c_str());
  auto time = g_random_int();
  nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp = time;

  InSequence seq;
  ON_CALL(*volume_, IsMounted()).WillByDefault(Return(false));
  EXPECT_CALL(*volume_, Mount());
  EXPECT_CALL(*file_manager_, CopyFiles(data.Uris(), volume_->GetUri(), time));
  icon_->AcceptDrop(data);
}

TEST_F(TestVolumeLauncherIcon, AcceptDropMounted)
{
  DndData data;
  std::string data_string = "file://file1\ndir2/file2\nfile://file3\nfile://dirN/fileN";
  data.Fill(data_string.c_str());
  auto time = g_random_int();
  nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp = time;

  InSequence seq;
  EXPECT_CALL(*volume_, Mount()).Times(0);
  EXPECT_CALL(*file_manager_, CopyFiles(data.Uris(), volume_->GetUri(), time));
  icon_->AcceptDrop(data);
}

}
