// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include <gmock/gmock.h>
using namespace testing;

#include "DeviceLauncherSection.h"
#include "DevicesSettings.h"
#include "AbstractVolumeMonitorWrapper.h"
using namespace unity;
using namespace unity::launcher;

#include "gmockvolume.h"
#include "test_utils.h"

namespace
{

class EventListener
{
public:
  EventListener()
    : icon_added(false)
  {}

  void OnIconAdded(AbstractLauncherIcon::Ptr icon)
  {
    icon_added = true;
  }

  bool icon_added;
};

class MockVolumeMonitorWrapper : public AbstractVolumeMonitorWrapper
{
public:
  typedef std::shared_ptr<MockVolumeMonitorWrapper> Ptr;

  MockVolumeMonitorWrapper()
    : volume1(G_VOLUME(g_mock_volume_new()))
    , volume2(G_VOLUME(g_mock_volume_new()))
  {
  }

  VolumeList GetVolumes()
  {
    VolumeList ret;

    ret.push_back(volume1);
    ret.push_back(volume2);

    return ret;
  }

  glib::Object<GVolume> volume1;
  glib::Object<GVolume> volume2;
};

class MockDevicesSettings : public DevicesSettings
{
  MOCK_CONST_METHOD1(IsABlacklistedDevice, bool(std::string const& uuid));
  MOCK_METHOD1(TryToBlacklist, void(std::string const& uuid));
  MOCK_METHOD1(TryToUnblacklist, void(std::string const& uuid));
};

class TestDeviceLauncherSection : public Test
{
public:
  TestDeviceLauncherSection()
    : monitor_(new MockVolumeMonitorWrapper)
    , devices_settings_(new MockDevicesSettings)
    , section_(monitor_, devices_settings_)
  {}

  void SetUp()
  {
    // Make sure PopulateEntries is called.
    Utils::WaitForTimeoutMSec(1500);
  }

  MockVolumeMonitorWrapper::Ptr monitor_;
  DevicesSettings::Ptr devices_settings_;
  DeviceLauncherSection section_;
};


TEST_F(TestDeviceLauncherSection, TestNoDuplicates)
{
  std::shared_ptr<EventListener> listener(new EventListener);
  section_.IconAdded.connect(sigc::mem_fun(*listener, &EventListener::OnIconAdded));

  // Emit the signal volume_added for each volume.
  monitor_->volume_added.emit(monitor_->volume1);
  monitor_->volume_added.emit(monitor_->volume2);

  Utils::WaitForTimeoutMSec(500);

  EXPECT_EQ(listener->icon_added, false);
}

}

