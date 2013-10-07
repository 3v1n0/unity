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

#include "DevicesSettings.h"
#include "test_mock_devices.h"
#include "test_utils.h"

using namespace testing;
using namespace unity::launcher;

namespace unity
{
namespace
{

struct MockDeviceNotificationDisplay : DeviceNotificationDisplay
{
  typedef NiceMock<MockDeviceNotificationDisplay> Nice;
  MOCK_METHOD2(Display, void(std::string const& icon_name, std::string const& volume_name));
};

class EventListener
{
public:
  EventListener()
    : icon_added(false)
  {}

  void OnIconAdded(AbstractLauncherIcon::Ptr const& icon)
  {
    icon_added = true;
  }

  bool icon_added;
};

struct TestDeviceLauncherSection : Test
{
  TestDeviceLauncherSection()
    : monitor_(std::make_shared<MockVolumeMonitorWrapper>())
    , devices_settings_(std::make_shared<NiceMock<MockDevicesSettings>>())
    , notifications_(std::make_shared<MockDeviceNotificationDisplay::Nice>())
    , section_(monitor_, devices_settings_, notifications_)
  {}

  MockVolumeMonitorWrapper::Ptr monitor_;
  DevicesSettings::Ptr devices_settings_;
  DeviceNotificationDisplay::Ptr notifications_;
  DeviceLauncherSection section_;
};

TEST_F(TestDeviceLauncherSection, NoDuplicates)
{
  std::shared_ptr<EventListener> listener(new EventListener);
  section_.icon_added.connect(sigc::mem_fun(*listener, &EventListener::OnIconAdded));

  // Emit the signal volume_added for each volume.
  monitor_->volume_added.emit(*(std::next(monitor_->volumes_.begin(), 0)));
  monitor_->volume_added.emit(*(std::next(monitor_->volumes_.begin(), 1)));

  Utils::WaitForTimeoutMSec(500);

  EXPECT_EQ(listener->icon_added, false);
}

TEST_F(TestDeviceLauncherSection, GetIcons)
{
  EXPECT_EQ(section_.GetIcons().size(), 2);
}

}
}

