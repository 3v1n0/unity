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

#ifndef TEST_MOCK_DEVICES_H
#define TEST_MOCK_DEVICES_H

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

struct TestDeviceLauncherSection : Test
{
  TestDeviceLauncherSection()
    : monitor_(new MockVolumeMonitorWrapper)
    , devices_settings_(new MockDevicesSettings)
    , section_(monitor_, devices_settings_)
  {}

  MockVolumeMonitorWrapper::Ptr monitor_;
  DevicesSettings::Ptr devices_settings_;
  DeviceLauncherSection section_;
};

TEST_F(TestDeviceLauncherSection, NoDuplicates)
{
  std::shared_ptr<EventListener> listener(new EventListener);
  section_.IconAdded.connect(sigc::mem_fun(*listener, &EventListener::OnIconAdded));

  // Emit the signal volume_added for each volume.
  monitor_->volume_added.emit(monitor_->volume1);
  monitor_->volume_added.emit(monitor_->volume2);

  Utils::WaitForTimeoutMSec(500);

  EXPECT_EQ(listener->icon_added, false);
}

TEST_F(TestDeviceLauncherSection, GetIcons)
{
  EXPECT_EQ(section_.GetIcons().size(), 2);
}

}
}

#endif
