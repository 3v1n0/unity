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

#include <gio/gio.h>

#include "FileManagerOpener.h"
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
  Impl(glib::Object<GVolume> const& volume,
       std::shared_ptr<FileManagerOpener> const& file_manager_opener)
    : cancellable_(g_cancellable_new())
    , volume_(volume)
    , file_manager_opener_(file_manager_opener)
  {}

  ~Impl()
  {
    g_cancellable_cancel(cancellable_);
  }

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
    if (!IsMounted())
      MountAndOnFinishOpenInFileManager();
    else
      OpenInFileManager();
  }

  void MountAndOnFinishOpenInFileManager()
  {
    g_volume_mount(volume_,
                   (GMountMountFlags) 0,
                   nullptr,
                   nullptr,
                   (GAsyncReadyCallback) &Impl::OnMountFinish,
                   this);
  }

  static void OnMountFinish(GObject* object,
                            GAsyncResult* result,
                            Impl* self)
  {
    if (g_volume_mount_finish(self->volume_, result, nullptr))
      self->OpenInFileManager();
  }

  void OpenInFileManager()
  {
    file_manager_opener_->Open(GetUri());
  }

  std::string GetUri()
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    glib::Object<GFile> root(g_mount_get_root(mount));

    if (root.IsType(G_TYPE_FILE))
      return glib::String(g_file_get_uri(root)).Str();
    else
     return std::string();
  }

  glib::Object<GCancellable> cancellable_;
  glib::Object<GVolume> volume_;
  std::shared_ptr<FileManagerOpener> file_manager_opener_;
};

//
// End private implementation
//

VolumeImpl::VolumeImpl(glib::Object<GVolume> const& volume,
                       std::shared_ptr<FileManagerOpener> const& file_manager_opener)
  : pimpl(new Impl(volume, file_manager_opener))
{}

VolumeImpl::~VolumeImpl()
{}

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
