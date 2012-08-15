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
#include "launcher/FileManagerOpener.h"
#include "test_utils.h"
using namespace unity;

namespace
{

class MockFileManagerOpener : public launcher::FileManagerOpener
{
public:
   MOCK_METHOD1(Open, void(std::string const& uri));
};


class TestVolumeImp : public Test
{
public:
  void SetUp()
  {
    gvolume_ = g_mock_volume_new();
    file_manager_opener_.reset(new MockFileManagerOpener);
    volume_.reset(new launcher::VolumeImp(glib::Object<GVolume>(G_VOLUME(gvolume_.RawPtr()), glib::AddRef()),
                                           file_manager_opener_));
  }

  glib::Object<GMockVolume> gvolume_;
  std::shared_ptr<MockFileManagerOpener> file_manager_opener_;
  std::unique_ptr<launcher::VolumeImp> volume_;
};

TEST_F(TestVolumeImp, TestCtor)
{
  EXPECT_FALSE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, TestGetName)
{
  std::string const volume_name("Test Device");

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
  std::string const uuid("0123456789abc");

  g_mock_volume_set_uuid(gvolume_, uuid.c_str());
  EXPECT_EQ(volume_->GetIdentifier(), uuid);
}

TEST_F(TestVolumeImp, TestIsMounted)
{
  g_mock_volume_set_mount(gvolume_, nullptr);
  ASSERT_FALSE(volume_->IsMounted());

  g_mock_volume_set_mount(gvolume_, G_MOUNT(g_mock_mount_new()));
  EXPECT_TRUE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, TestMountAndOpenInFileManager)
{
  EXPECT_CALL(*file_manager_opener_, Open("file:///some/directory/testfile"))
      .Times(1);

  volume_->MountAndOpenInFileManager();
  EXPECT_TRUE(volume_->IsMounted());

  EXPECT_CALL(*file_manager_opener_, Open("file:///some/directory/testfile"))
      .Times(1);

  volume_->MountAndOpenInFileManager();
  EXPECT_TRUE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, TestChangedSignal)
{
  bool callback_called = false;
  volume_->changed.connect([&]()
  {
    callback_called = true;
  });

  g_signal_emit_by_name(gvolume_, "changed", nullptr);

  Utils::WaitUntil(callback_called);

  EXPECT_TRUE(callback_called);
}

}
