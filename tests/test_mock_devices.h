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

#include "DeviceLauncherSection.h"
#include "AbstractVolumeMonitorWrapper.h"
#include "Volume.h"
#include "gmockvolume.h"

using namespace unity::launcher;

namespace unity
{
namespace
{

struct MockVolumeMonitorWrapper : public AbstractVolumeMonitorWrapper
{
  typedef std::shared_ptr<MockVolumeMonitorWrapper> Ptr;

  MockVolumeMonitorWrapper(unsigned size = 2)
  {
    for (unsigned i = 0; i < size; ++i)
    {
      glib::Object<GVolume> volume(G_VOLUME(g_mock_volume_new()));
      volumes_.push_back(volume);
    }
  }

  VolumeList GetVolumes() { return volumes_; }

  VolumeList volumes_;
};

struct MockDevicesSettings : DevicesSettings
{
  typedef std::shared_ptr<MockDevicesSettings> Ptr;
  typedef testing::NiceMock<MockDevicesSettings> Nice;

  MOCK_CONST_METHOD1(IsABlacklistedDevice, bool(std::string const& uuid));
  MOCK_METHOD1(TryToBlacklist, void(std::string const& uuid));
  MOCK_METHOD1(TryToUnblacklist, void(std::string const& uuid));
};

struct MockDeviceLauncherSection : DeviceLauncherSection
{
  MockDeviceLauncherSection(unsigned size = 2)
    : DeviceLauncherSection(MockVolumeMonitorWrapper::Ptr(new testing::NiceMock<MockVolumeMonitorWrapper>(size)),
                            DevicesSettings::Ptr(new testing::NiceMock<MockDevicesSettings>))
  {}
};

struct MockVolume : Volume
{
  typedef std::shared_ptr<MockVolume> Ptr;
  typedef testing::NiceMock<MockVolume> Nice;

  MOCK_CONST_METHOD0(CanBeRemoved, bool(void));
  MOCK_CONST_METHOD0(CanBeStopped, bool(void));
  MOCK_CONST_METHOD0(GetName, std::string(void));
  MOCK_CONST_METHOD0(GetIconName, std::string(void));
  MOCK_CONST_METHOD0(GetIdentifier, std::string(void));
  MOCK_CONST_METHOD0(GetUri, std::string(void));
  MOCK_CONST_METHOD0(HasSiblings, bool(void));
  MOCK_CONST_METHOD0(CanBeEjected, bool(void));
  MOCK_CONST_METHOD0(IsMounted, bool(void));

  MOCK_METHOD0(Eject, void());
  MOCK_METHOD0(Mount, void());
  MOCK_METHOD0(StopDrive, void(void));
  MOCK_METHOD0(Unmount, void(void));
};

struct MockDeviceNotificationDisplay : DeviceNotificationDisplay
{
  typedef std::shared_ptr<MockDeviceNotificationDisplay> Ptr;
  typedef testing::NiceMock<MockDeviceNotificationDisplay> Nice;

  MOCK_METHOD2(Display, void(std::string const& icon_name, std::string const& volume_name));
};

} // anonymous namespace
} // unity namespace

#endif
