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

#include "DeviceLauncherSection.h"
#include "AbstractVolumeMonitorWrapper.h"
#include "gmockvolume.h"

using namespace unity::launcher;

namespace unity
{

struct MockVolumeMonitorWrapper : public AbstractVolumeMonitorWrapper
{
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

struct MockDevicesSettings : DevicesSettings
{
  typedef std::shared_ptr<MockDevicesSettings> Ptr;

  MOCK_CONST_METHOD1(IsABlacklistedDevice, bool(std::string const& uuid));
  MOCK_METHOD1(TryToBlacklist, void(std::string const& uuid));
  MOCK_METHOD1(TryToUnblacklist, void(std::string const& uuid));
};

struct MockDeviceLauncherSection : DeviceLauncherSection
{
  MockDeviceLauncherSection()
    : DeviceLauncherSection(MockVolumeMonitorWrapper::Ptr(new MockVolumeMonitorWrapper),
                            DevicesSettings::Ptr(new MockDevicesSettings))
  {}
};

}
