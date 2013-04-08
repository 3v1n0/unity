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

#include <memory>

#include <gmock/gmock.h>
using namespace testing;

#include "gmockmount.h"
#include "gmockvolume.h"
#include "launcher/VolumeImp.h"
#include "test_utils.h"
#include "test_mock_filemanager.h"
using namespace unity;

namespace
{

class MockDeviceNotificationDisplay : public launcher::DeviceNotificationDisplay
{
public:
  typedef std::shared_ptr<MockDeviceNotificationDisplay> Ptr;

  MOCK_METHOD2(Display, void(std::string const& icon_name, std::string const& device_name));
};

class TestVolumeImp : public Test
{
public:
  void SetUp()
  {
    gvolume_ = g_mock_volume_new();
    file_manager_.reset(new NiceMock<MockFileManager>);
    device_notification_display_.reset(new MockDeviceNotificationDisplay);
    volume_.reset(new launcher::VolumeImp(glib::Object<GVolume>(G_VOLUME(gvolume_.RawPtr()), glib::AddRef()),
                                          file_manager_, device_notification_display_));
  }

  glib::Object<GMockVolume> gvolume_;
  MockFileManager::Ptr file_manager_;
  MockDeviceNotificationDisplay::Ptr device_notification_display_;
  launcher::VolumeImp::Ptr volume_;
};

TEST_F(TestVolumeImp, TestCtor)
{
  EXPECT_FALSE(volume_->IsMounted());
  EXPECT_FALSE(volume_->IsOpened());
}

TEST_F(TestVolumeImp, TestCanBeEjected)
{
  EXPECT_FALSE(volume_->CanBeEjected());

  g_mock_volume_set_can_eject(gvolume_, TRUE);
  EXPECT_TRUE(volume_->CanBeEjected());
}

TEST_F(TestVolumeImp, TestGetName)
{
  std::string const volume_name("Test Device");

  // g_mock_volume_set_name is equivalent to
  // EXPECT_CALL(gvolume_, g_volume_get_name) ...
  g_mock_volume_set_name(gvolume_, volume_name.c_str());
  EXPECT_EQ(volume_->GetName(), volume_name);
}

TEST_F(TestVolumeImp, TestGetIconName)
{
  std::string const icon_name("gnome-dev-cdrom");

  g_mock_volume_set_icon(gvolume_, g_icon_new_for_string(icon_name.c_str(), NULL));
  EXPECT_EQ(volume_->GetIconName(), icon_name);
}

TEST_F(TestVolumeImp, TestGetIdentifier)
{
  std::string const uuid = "uuid";
  std::string const label = "label";

  g_mock_volume_set_uuid(gvolume_, uuid.c_str());
  g_mock_volume_set_label(gvolume_, label.c_str());

  EXPECT_EQ(volume_->GetIdentifier(), uuid + "-" + label);
}

TEST_F(TestVolumeImp, TestIsMounted)
{
  g_mock_volume_set_mount(gvolume_, nullptr);
  ASSERT_FALSE(volume_->IsMounted());

  g_mock_volume_set_mount(gvolume_, G_MOUNT(g_mock_mount_new()));
  EXPECT_TRUE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, TestIsOpened)
{
  volume_->MountAndOpenInFileManager(0);

  EXPECT_CALL(*file_manager_, IsPrefixOpened(ROOT_FILE_URI));
  ON_CALL(*file_manager_, IsPrefixOpened(_)).WillByDefault(Return(true));
  file_manager_->locations_changed.emit();
  EXPECT_TRUE(volume_->IsOpened());

  EXPECT_CALL(*file_manager_, IsPrefixOpened(ROOT_FILE_URI));
  ON_CALL(*file_manager_, IsPrefixOpened(_)).WillByDefault(Return(false));
  file_manager_->locations_changed.emit();
  EXPECT_FALSE(volume_->IsOpened());
}

TEST_F(TestVolumeImp, TestIsOpenedSignal)
{
  ON_CALL(*file_manager_, IsPrefixOpened(_)).WillByDefault(Return(false));

  bool opened = false;
  volume_->opened.connect([&opened] (bool value) { opened = value; });
  file_manager_->locations_changed.emit();

  ASSERT_FALSE(opened);

  ON_CALL(*file_manager_, IsPrefixOpened(_)).WillByDefault(Return(true));
  file_manager_->locations_changed.emit();
  EXPECT_TRUE(opened);
}

TEST_F(TestVolumeImp, TestFilemanagerSignalDisconnection)
{
  ASSERT_FALSE(file_manager_->locations_changed.empty());
  volume_.reset();

  EXPECT_TRUE(file_manager_->locations_changed.empty());
}

TEST_F(TestVolumeImp, TestEjectAndShowNotification)
{
  g_mock_volume_set_can_eject(gvolume_, TRUE);

  EXPECT_CALL(*device_notification_display_, Display(volume_->GetIconName(), volume_->GetName()))
    .Times(1);

  volume_->EjectAndShowNotification();
}

TEST_F(TestVolumeImp, TestMountAndOpenInFileManager)
{
  unsigned long long time = g_random_int();
  EXPECT_CALL(*file_manager_, OpenActiveChild(ROOT_FILE_URI, time));

  volume_->MountAndOpenInFileManager(time);
  EXPECT_EQ(g_mock_volume_last_mount_had_mount_operation(gvolume_), TRUE);
  EXPECT_TRUE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, TestChangedSignal)
{
  bool callback_called = false;
  volume_->changed.connect([&]() {
    callback_called = true;
  });

  g_signal_emit_by_name(gvolume_, "changed", nullptr);
  Utils::WaitUntil(callback_called);
}

TEST_F(TestVolumeImp, TestRemovedSignal)
{
  bool callback_called = false;
  volume_->removed.connect([&]() {
    callback_called = true;
  });

  g_signal_emit_by_name(gvolume_, "removed", nullptr);
  Utils::WaitUntil(callback_called);
}

}
