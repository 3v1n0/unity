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
using namespace unity;

namespace
{
struct TestVolumeImp : Test
{
  TestVolumeImp()
    : gvolume_(g_mock_volume_new())
    , volume_(std::make_shared<launcher::VolumeImp>(glib::object_cast<GVolume>(gvolume_)))
  {}

  glib::Object<GMockVolume> gvolume_;
  launcher::VolumeImp::Ptr volume_;
};

TEST_F(TestVolumeImp, Ctor)
{
  EXPECT_FALSE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, CanBeEjected)
{
  EXPECT_FALSE(volume_->CanBeEjected());

  g_mock_volume_set_can_eject(gvolume_, TRUE);
  EXPECT_TRUE(volume_->CanBeEjected());
}

TEST_F(TestVolumeImp, GetName)
{
  std::string const volume_name("Test Device");

  // g_mock_volume_set_name is equivalent to
  // EXPECT_CALL(gvolume_, g_volume_get_name) ...
  g_mock_volume_set_name(gvolume_, volume_name.c_str());
  EXPECT_EQ(volume_->GetName(), volume_name);
}

TEST_F(TestVolumeImp, GetIconName)
{
  std::string const icon_name("gnome-dev-cdrom");

  g_mock_volume_set_icon(gvolume_, g_icon_new_for_string(icon_name.c_str(), NULL));
  EXPECT_EQ(volume_->GetIconName(), icon_name);
}

TEST_F(TestVolumeImp, GetIdentifier)
{
  std::string const uuid = "uuid";
  std::string const label = "label";

  g_mock_volume_set_uuid(gvolume_, uuid.c_str());
  g_mock_volume_set_label(gvolume_, label.c_str());

  EXPECT_EQ(volume_->GetIdentifier(), uuid + "-" + label);
}

TEST_F(TestVolumeImp, GetUriUnMounted)
{
  EXPECT_TRUE(volume_->GetUri().empty());
}

TEST_F(TestVolumeImp, IsMounted)
{
  g_mock_volume_set_mount(gvolume_, nullptr);
  ASSERT_FALSE(volume_->IsMounted());

  g_mock_volume_set_mount(gvolume_, G_MOUNT(g_mock_mount_new()));
  EXPECT_TRUE(volume_->IsMounted());
}

TEST_F(TestVolumeImp, Eject)
{
  bool ejected = false;
  g_mock_volume_set_can_eject(gvolume_, TRUE);
  volume_->ejected.connect([&ejected] { ejected = true; });
  volume_->Eject();
  EXPECT_TRUE(ejected);
}

TEST_F(TestVolumeImp, Mount)
{
  bool mounted = false;
  volume_->mounted.connect([&mounted] { mounted = true; });
  volume_->Mount();
  EXPECT_EQ(g_mock_volume_last_mount_had_mount_operation(gvolume_), TRUE);
  EXPECT_TRUE(volume_->IsMounted());
  EXPECT_TRUE(mounted);
}

TEST_F(TestVolumeImp, ChangedSignal)
{
  bool callback_called = false;
  volume_->changed.connect([&]() {
    callback_called = true;
  });

  g_signal_emit_by_name(gvolume_, "changed", nullptr);
  EXPECT_TRUE(callback_called);
}

TEST_F(TestVolumeImp, RemovedSignal)
{
  bool callback_called = false;
  volume_->removed.connect([&]() {
    callback_called = true;
  });

  g_signal_emit_by_name(gvolume_, "removed", nullptr);
  EXPECT_TRUE(callback_called);
}

}
