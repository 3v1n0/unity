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

#include "VolumeImp.h"

namespace unity
{
namespace launcher
{

//
// Start private implementation
//

class VolumeImp::Impl : public sigc::trackable
{
public:
  Impl(glib::Object<GVolume> const& volume,
       FileManager::Ptr const& file_manager,
       DeviceNotificationDisplay::Ptr const& device_notification_display,
       VolumeImp* parent)
    : parent_(parent)
    , opened_(false)
    , open_timestamp_(0)
    , volume_(volume)
    , file_manager_(file_manager)
    , device_notification_display_(device_notification_display)
  {
    signal_volume_changed_.Connect(volume_, "changed", [this] (GVolume*) {
      parent_->changed.emit();
    });

    signal_volume_removed_.Connect(volume_, "removed", [this] (GVolume*) {
      parent_->removed.emit();
    });

    file_manager_->locations_changed.connect(sigc::mem_fun(this, &Impl::OnLocationChanged));
  }

  void OnLocationChanged()
  {
    bool opened = file_manager_->IsPrefixOpened(GetUri());

    if (opened_ != opened)
    {
      opened_ = opened;
      parent_->opened.emit(opened_);
    }
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
    glib::String label(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_LABEL));
    glib::String uuid(g_volume_get_identifier(volume_, G_VOLUME_IDENTIFIER_KIND_UUID));

    return uuid.Str() + "-" + label.Str();
  }

  bool HasSiblings() const
  {
    glib::Object<GDrive> drive(g_volume_get_drive(volume_));

    if (!drive)
      return false;

    GList* volumes = g_drive_get_volumes(drive);
    bool has_sibilings = volumes && volumes->next;

    if (volumes)
      g_list_free_full(volumes, g_object_unref);

    return has_sibilings;
  }

  bool IsMounted() const
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    return static_cast<bool>(mount);
  }

  bool IsOpened() const
  {
    return opened_;
  }

  void EjectAndShowNotification()
  {
    if (!CanBeEjected())
      return;

    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(nullptr));

    g_volume_eject_with_operation(volume_,
                                  (GMountUnmountFlags)0,
                                  mount_op,
                                  cancellable_,
                                  (GAsyncReadyCallback)OnEjectReady,
                                  this);
  }

  static void OnEjectReady(GObject* object, GAsyncResult* result, Impl* self)
  {
    if (g_volume_eject_with_operation_finish(G_VOLUME(object), result, nullptr))
    {
      self->parent_->ejected.emit();
      self->device_notification_display_->Display(self->GetIconName(), self->GetName());
    }
  }

  void MountAndOpenInFileManager(uint64_t timestamp)
  {
    open_timestamp_ = timestamp;

    if (!IsMounted())
      MountAndOnFinishOpenInFileManager();
    else
      OpenInFileManager();
  }

  void MountAndOnFinishOpenInFileManager()
  {
    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(nullptr));

    g_volume_mount(volume_,
                   (GMountMountFlags) 0,
                   mount_op,
                   cancellable_,
                   (GAsyncReadyCallback) &Impl::OnMountFinish,
                   this);
  }

  static void OnMountFinish(GObject* object,
                            GAsyncResult* result,
                            Impl* self)
  {
    if (g_volume_mount_finish(G_VOLUME(object), result, nullptr))
    {
      self->parent_->mounted.emit();
      self->OpenInFileManager();
    }
  }

  void OpenInFileManager()
  {
    file_manager_->OpenActiveChild(GetUri(), open_timestamp_);
  }

  std::string GetUri() const
  {
    glib::Object<GMount> mount(g_volume_get_mount(volume_));

    if (!mount.IsType(G_TYPE_MOUNT))
      return std::string();

    glib::Object<GFile> root(g_mount_get_root(mount));

    if (!root.IsType(G_TYPE_FILE))
      return std::string();

    return glib::String(g_file_get_uri(root)).Str();
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
                 cancellable_, nullptr, nullptr);
  }

  void Unmount()
  {
    if (!IsMounted())
      return;

    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    glib::Object<GMountOperation> op(gtk_mount_operation_new(nullptr));

    g_mount_unmount_with_operation(mount, (GMountUnmountFlags) 0, op, cancellable_,
                                   FinishUmount, this);
  }

  static void FinishUmount(GObject* object, GAsyncResult* res, gpointer data)
  {
    if (g_mount_unmount_with_operation_finish(G_MOUNT(object), res, nullptr))
      static_cast<Impl*>(data)->parent_->unmounted.emit();
  }

  VolumeImp* parent_;
  bool opened_;
  uint64_t open_timestamp_;
  glib::Cancellable cancellable_;
  glib::Object<GVolume> volume_;
  FileManager::Ptr file_manager_;
  DeviceNotificationDisplay::Ptr device_notification_display_;

  glib::Signal<void, GVolume*> signal_volume_changed_;
  glib::Signal<void, GVolume*> signal_volume_removed_;
};

//
// End private implementation
//

VolumeImp::VolumeImp(glib::Object<GVolume> const& volume,
                     FileManager::Ptr const& file_manager,
                     DeviceNotificationDisplay::Ptr const& device_notification_display)
  : pimpl(new Impl(volume, file_manager, device_notification_display, this))
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

bool VolumeImp::IsOpened() const
{
  return pimpl->IsOpened();
}

void VolumeImp::MountAndOpenInFileManager(uint64_t timestamp)
{
  pimpl->MountAndOpenInFileManager(timestamp);
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
