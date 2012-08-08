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

#include <gtest/gtest.h>
using namespace testing;

#include "gmockmount.h"
#include "gmockvolume.h"
#include "launcher/VolumeImpl.h"
#include "test_utils.h"
using namespace unity;

namespace
{

class TestVolumeImpl : public Test
{
public:
  void SetUp()
  {
    gvolume_ = g_mock_volume_new();
    volume_.reset(new launcher::VolumeImpl(glib::Object<GVolume>(G_VOLUME(gvolume_.RawPtr()))));
  }

  glib::Object<GMockVolume> gvolume_;
  std::unique_ptr<launcher::VolumeImpl> volume_;
};

TEST_F(TestVolumeImpl, TestGetName)
{
  std::string const volume_name("Test Device");

  g_mock_volume_set_name(gvolume_, volume_name.c_str());
  EXPECT_EQ(volume_->GetName(), volume_name);
}

TEST_F(TestVolumeImpl, TestGetIconName)
{
  std::string const icon_name("gnome-dev-cdrom");

  g_mock_volume_set_icon(gvolume_, g_icon_new_for_string(icon_name.c_str(), NULL));
  EXPECT_EQ(volume_->GetIconName(), icon_name);
}

TEST_F(TestVolumeImpl, TestGetIdentifer)
{
  std::string const uuid("0123456789abc");

  g_mock_volume_set_uuid(gvolume_, uuid.c_str());
  EXPECT_EQ(volume_->GetIdentifer(), uuid);
}

TEST_F(TestVolumeImpl, TestIsMounted)
{
  g_mock_volume_set_mount(gvolume_, nullptr);
  ASSERT_FALSE(volume_->IsMounted());

  g_mock_volume_set_mount(gvolume_, G_MOUNT(g_mock_mount_new()));
  EXPECT_TRUE(volume_->IsMounted());
}

}
