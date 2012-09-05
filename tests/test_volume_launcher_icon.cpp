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

#include "launcher/DevicesSettings.h"
#include "launcher/Volume.h"
#include "launcher/VolumeLauncherIcon.h"
#include "test_utils.h"
using namespace unity;
using namespace unity::launcher;

namespace
{

class MockVolume : public Volume
{
public:
  typedef std::shared_ptr<MockVolume> Ptr;

  MOCK_CONST_METHOD0(CanBeRemoved, bool(void));
  MOCK_CONST_METHOD0(CanBeStopped, bool(void));
  MOCK_CONST_METHOD0(GetName, std::string(void));
  MOCK_CONST_METHOD0(GetIconName, std::string(void));
  MOCK_CONST_METHOD0(GetIdentifier, std::string(void));
  MOCK_CONST_METHOD0(HasSiblings, bool(void));
  MOCK_CONST_METHOD0(CanBeEjected, bool(void));
  MOCK_CONST_METHOD0(IsMounted, bool(void));

  MOCK_METHOD0(EjectAndShowNotification, void(void));
  MOCK_METHOD0(MountAndOpenInFileManager, void(void));
  MOCK_METHOD0(StopDrive, void(void));
  MOCK_METHOD0(Unmount, void(void));
};

class MockDevicesSettings : public DevicesSettings
{
public:
  typedef std::shared_ptr<MockDevicesSettings> Ptr;

  MOCK_CONST_METHOD1(IsABlacklistedDevice, bool(std::string const& uuid));
  MOCK_METHOD1(TryToBlacklist, void(std::string const& uuid));
  MOCK_METHOD1(TryToUnblacklist, void(std::string const& uuid));
};


class TestVolumeLauncherIcon : public Test
{
public:
  virtual void SetUp()
  {
    volume_.reset(new MockVolume);
    settings_.reset(new MockDevicesSettings);

    SetupVolumeDefaultBehavior();
    SetupSettingsDefaultBehavior();
  }

  void CreateIcon()
  {
    icon_ = new VolumeLauncherIcon(volume_, settings_);
  }

  void SetupSettingsDefaultBehavior()
  {
    EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
      .WillRepeatedly(Return(false));
  }

  void SetupVolumeDefaultBehavior()
  {
    EXPECT_CALL(*volume_, CanBeRemoved())
      .WillRepeatedly(Return(false));

    EXPECT_CALL(*volume_, CanBeStopped())
      .WillRepeatedly(Return(false));

    EXPECT_CALL(*volume_, GetName())
      .WillRepeatedly(Return("Test Name"));

    EXPECT_CALL(*volume_, GetIconName())
      .WillRepeatedly(Return("Test Icon Name"));

    EXPECT_CALL(*volume_, GetIdentifier())
      .WillRepeatedly(Return("Test Identifier"));

    EXPECT_CALL(*volume_, HasSiblings())
      .WillRepeatedly(Return(false));

    EXPECT_CALL(*volume_, CanBeEjected())
      .WillRepeatedly(Return(false));

    EXPECT_CALL(*volume_, IsMounted())
      .WillRepeatedly(Return(true));
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
  VolumeLauncherIcon::Ptr icon_;
};

TEST_F(TestVolumeLauncherIcon, TestIconType)
{
  CreateIcon();
  EXPECT_EQ(icon_->GetIconType(), AbstractLauncherIcon::IconType::DEVICE);
}

TEST_F(TestVolumeLauncherIcon, TestQuirks)
{
  CreateIcon();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestVolumeLauncherIcon, TestTooltipText)
{
  CreateIcon();

  ASSERT_EQ(icon_->tooltip_text, "Test Name");
}

TEST_F(TestVolumeLauncherIcon, TestIconName)
{
  CreateIcon();

  ASSERT_EQ(icon_->icon_name, "Test Icon Name");
}

TEST_F(TestVolumeLauncherIcon, TestVisibility_InitiallyMountedVolume)
{
  CreateIcon();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibility_InitiallyMountedBlacklistedVolume)
{
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  CreateIcon();

  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}


TEST_F(TestVolumeLauncherIcon, TestVisibility_InitiallyUnmountedVolume)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  CreateIcon();

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}


TEST_F(TestVolumeLauncherIcon, TestVisibility_InitiallyUnmountedBlacklistedVolume)
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
  CreateIcon();

  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));
  settings_->changed.emit();

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibilityAfterUnmount)
{
  CreateIcon();

  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);

  volume_->changed.emit();
  Utils::WaitForTimeout(1);

  EXPECT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestVisibilityAfterUnmount_BlacklistedVolume)
{
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  CreateIcon();

  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(0);

  volume_->changed.emit();
  Utils::WaitForTimeout(1);

  EXPECT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_VolumeWithoutIdentifier)
{
  EXPECT_CALL(*volume_, GetIdentifier())
    .WillRepeatedly(Return(""));

  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_Success)
{
  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(0);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(1);

  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  settings_->changed.emit(); // TryToBlacklist() works if DevicesSettings emits a changed signal.
  Utils::WaitForTimeout(1);

  ASSERT_FALSE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestUnlockFromLauncherMenuItem_Failure)
{
  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(0);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unlock from Launcher");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(1);

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  Utils::WaitForTimeout(1);

  ASSERT_TRUE(icon_->GetQuirk(AbstractLauncherIcon::Quirk::VISIBLE));
}

TEST_F(TestVolumeLauncherIcon, TestOpenMenuItem)
{
  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Open");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  EXPECT_CALL(*volume_, MountAndOpenInFileManager())
    .Times(1);

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  Utils::WaitForTimeout(1);
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem_NotEjectableVolume)
{
  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Eject");
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));

  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(2);

  EXPECT_CALL(*volume_, EjectAndShowNotification())
    .Times(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Eject");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  Utils::WaitForTimeout(1);
}

TEST_F(TestVolumeLauncherIcon, TestEjectMenuItem_NotStoppableVolume)
{
  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Safely remove");
}

TEST_F(TestVolumeLauncherIcon, TestSafelyRemoveMenuItem)
{
  EXPECT_CALL(*volume_, CanBeStopped())
    .WillRepeatedly(Return(true));

  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(2);

  EXPECT_CALL(*volume_, StopDrive())
    .Times(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Safely remove");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  Utils::WaitForTimeout(1);
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_UnmountedVolume)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(false));

  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}


TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_EjectableVolume)
{
  EXPECT_CALL(*volume_, CanBeEjected())
    .WillRepeatedly(Return(true));

  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(true));

  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem_StoppableVolume)
{
  EXPECT_CALL(*volume_, CanBeStopped())
    .WillRepeatedly(Return(true));

  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(true));

  CreateIcon();

  for (auto menuitem : icon_->GetMenus())
    ASSERT_STRNE(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
}

TEST_F(TestVolumeLauncherIcon, TestUnmountMenuItem)
{
  EXPECT_CALL(*volume_, IsMounted())
    .WillRepeatedly(Return(true));

  CreateIcon();

  auto menuitem = GetMenuItemAtIndex(2);

  EXPECT_CALL(*volume_, Unmount())
    .Times(1);

  ASSERT_STREQ(dbusmenu_menuitem_property_get(menuitem, DBUSMENU_MENUITEM_PROP_LABEL), "Unmount");
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE));
  EXPECT_TRUE(dbusmenu_menuitem_property_get_bool(menuitem, DBUSMENU_MENUITEM_PROP_ENABLED));

  dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, 0);
  Utils::WaitForTimeout(1);
}

TEST_F(TestVolumeLauncherIcon, TestCanBeEject)
{
  CreateIcon();

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

  CreateIcon();

  EXPECT_CALL(*volume_, EjectAndShowNotification())
    .Times(1);

  icon_->EjectAndShowNotification();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved)
{
  CreateIcon();

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(0);

  volume_->removed.emit();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved_RemovabledVolume)
{
  EXPECT_CALL(*volume_, CanBeRemoved())
    .WillRepeatedly(Return(true));
  CreateIcon();

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(0);

  volume_->removed.emit();
}

TEST_F(TestVolumeLauncherIcon, OnRemoved_RemovableAndBlacklistedVolume)
{
  EXPECT_CALL(*volume_, CanBeRemoved())
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*settings_, IsABlacklistedDevice(_))
    .WillRepeatedly(Return(true));
  CreateIcon();

  EXPECT_CALL(*settings_, TryToBlacklist(_))
    .Times(0);
  EXPECT_CALL(*settings_, TryToUnblacklist(_))
    .Times(1);

  volume_->removed.emit();
}

}
