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
#include <gtk/gtk.h>
#include <UnityCore/GLibSignal.h>

#include "DeviceNotificationShower.h"
#include "FileManagerOpener.h"
#include "VolumeImp.h"

namespace unity
{
namespace launcher
{

//
// Start private implementation
//

class VolumeImp::Impl
{
public:
  Impl(glib::Object<GVolume> const& volume,
       std::shared_ptr<FileManagerOpener> const& file_manager_opener,
       std::shared_ptr<DeviceNotificationShower> const& device_notification_shower,
       VolumeImp* parent)
    : parent_(parent)
    , cancellable_(g_cancellable_new())
    , volume_(volume)
    , file_manager_opener_(file_manager_opener)
    , device_notification_shower_(device_notification_shower)
  {
    signal_volume_changed_.Connect(volume_, "changed", [this] (GVolume*) {
      parent_->changed.emit();
    });
  }

  ~Impl()
  {
    g_cancellable_cancel(cancellable_);
  }

  bool CanBeEjected() const
  {
    return g_volume_can_eject(volume_) != FALSE;
  }

  bool CanBeRemoved() const
  {
    glib::Object<GDrive> drive(g_volume_get_drive(volume_));
    return drive && g_drive_is_media_removable(drive) != FALSE;
  }

  bool CanBeStopped() const
  {
    glib::Object<GDrive> drive(g_volume_get_drive(volume_));
    return drive && g_drive_can_stop(drive) != FALSE;
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

  std::string GetIdentifier() const
  {
    return glib::String(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID)).Str();
  }

  bool HasSiblings() const
  {
    glib::Object<GDrive> drive(g_volume_get_drive(volume_));

    if (!drive)
      return false;

    GList* volumes = g_drive_get_volumes(drive);
    bool has_sibilings = volumes && g_list_length(volumes) > 1;

    if (volumes)
      g_list_free_full(volumes, g_object_unref);

    return has_sibilings;
  }

  bool IsMounted() const
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    return static_cast<bool>(mount);
  }

  void EjectAndShowNotification()
  {
    if (!CanBeEjected())
      return;

    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(nullptr));

    g_volume_eject_with_operation(volume_,
                                  (GMountUnmountFlags)0,
                                  mount_op,
                                  nullptr,
                                  (GAsyncReadyCallback)OnEjectReady,
                                  this);
  }

  static void OnEjectReady(GObject* object, GAsyncResult* result, Impl* self)
  {
    if (g_volume_eject_with_operation_finish(self->volume_, result, nullptr))
    {
      self->device_notification_shower_->Show(self->GetIconName(), self->GetName());
    }
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

  void StopDrive()
  {
    if (!CanBeStopped())
      return;

    glib::Object<GDrive> drive(g_volume_get_drive(volume_));
    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(NULL));

    g_drive_stop(drive,
                 (GMountUnmountFlags)0,
                 mount_op,
                 nullptr, nullptr, nullptr);
  }

  void Unmount()
  {
    if (!IsMounted())
      return;

    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    glib::Object<GMountOperation> op(gtk_mount_operation_new(nullptr));

    g_mount_unmount_with_operation(mount,
                                   (GMountUnmountFlags)0,
                                   op,
                                   nullptr, nullptr, nullptr);
  }

  VolumeImp* parent_;
  glib::Object<GCancellable> cancellable_;
  glib::Object<GVolume> volume_;
  std::shared_ptr<FileManagerOpener> file_manager_opener_;
  std::shared_ptr<DeviceNotificationShower> device_notification_shower_;

  glib::Signal<void, GVolume*> signal_volume_changed_;
};

//
// End private implementation
//

VolumeImp::VolumeImp(glib::Object<GVolume> const& volume,
                     std::shared_ptr<FileManagerOpener> const& file_manager_opener,
                     std::shared_ptr<DeviceNotificationShower> const& device_notification_shower)
  : pimpl(new Impl(volume, file_manager_opener, device_notification_shower, this))
{}

VolumeImp::~VolumeImp()
{}

bool VolumeImp::CanBeEjected() const
{
  return pimpl->CanBeEjected();
}

bool VolumeImp::CanBeRemoved() const
{
  return pimpl->CanBeRemoved();
}

bool VolumeImp::CanBeStopped() const
{
  return pimpl->CanBeStopped();
}

std::string VolumeImp::GetName() const
{
  return pimpl->GetName();
}

std::string VolumeImp::GetIconName() const
{
  return pimpl->GetIconName();
}

std::string VolumeImp::GetIdentifier() const
{
  return pimpl->GetIdentifier();
}

bool VolumeImp::HasSiblings() const
{
  return pimpl->HasSiblings();
}

bool VolumeImp::IsMounted() const
{
  return pimpl->IsMounted();
}

void VolumeImp::MountAndOpenInFileManager()
{
  pimpl->MountAndOpenInFileManager();
}

void VolumeImp::EjectAndShowNotification()
{
  pimpl->EjectAndShowNotification();
}

void VolumeImp::StopDrive()
{
  pimpl->StopDrive();
}

void VolumeImp::Unmount()
{
  pimpl->Unmount();
}

} // namespace launcher
} // namespace unity
