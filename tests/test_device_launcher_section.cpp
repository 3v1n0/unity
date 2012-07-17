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
#include "VolumeMonitorWrapper.h"
using namespace unity;
using namespace unity::launcher;

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

class TestDeviceLauncherSection : public Test
{
public:
  TestDeviceLauncherSection()
    : monitor_(new VolumeMonitorWrapper)
    , section_(monitor_)
  {}

  void SetUp()
  {
    // Make sure PopulateEntries is called.
    Utils::WaitForTimeoutMSec(1500);
  }

  VolumeMonitorWrapper::Ptr monitor_;
  DeviceLauncherSection section_;
};


TEST_F(TestDeviceLauncherSection, TestNoDuplicates)
{
  std::shared_ptr<EventListener> listener(new EventListener);
  section_.IconAdded.connect(sigc::mem_fun(*listener, &EventListener::OnIconAdded));

  // Emit the signal volume_added for each volume.
  for (auto volume : monitor_->GetVolumes())
    monitor_->volume_added.emit(volume);

  Utils::WaitForTimeoutMSec(500);

  EXPECT_EQ(listener->icon_added, false);
}

}

