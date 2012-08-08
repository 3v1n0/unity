// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "VolumeImpl.h"

namespace unity
{
namespace launcher
{

//
// Start private implementation
//

class VolumeImpl::Impl
{
public:
  Impl(glib::Object<GVolume> const& volume)
    : volume_(volume)
  {}

  std::string GetName() const
  {
    return glib::String(g_volume_get_name(volume_)).Str();
  }

  std::string GetIconName() const
  {
    glib::Object<GIcon> icon(g_volume_get_icon(volume_));
    return glib::String(g_icon_to_string(icon)).Str();
  }

  std::string GetIdentifer() const
  {
    return glib::String(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID)).Str();
  }

  bool IsMounted() const
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    return static_cast<bool>(mount);
  }

  void MountAndOpenInFileManager()
  {
    if (not IsMounted())
      Mount();
    OpenInFileManager();
  }

  void Mount()
  {
    // TODO: Implement it
  }

  void OpenInFileManager()
  {
    // TODO: Implement it
  }
  
  glib::Object<GVolume> volume_;
};

//
// End private implementation
// 

VolumeImpl::VolumeImpl(glib::Object<GVolume> const& volume)
  : pimpl(new Impl(volume))
{}

VolumeImpl::~VolumeImpl()
{
  // For some weird reasons there are building issues removing this
  // apparently useless dtor.
}

std::string VolumeImpl::GetName() const
{
  return pimpl->GetName();
}

std::string VolumeImpl::GetIconName() const
{
  return pimpl->GetIconName();
}

std::string VolumeImpl::GetIdentifer() const
{
  return pimpl->GetIdentifer();
}

bool VolumeImpl::IsMounted() const
{
  return pimpl->IsMounted();
}

void VolumeImpl::MountAndOpenInFileManager()
{
  return pimpl->MountAndOpenInFileManager();
}

} // namespace launcher
} // namespace unity
