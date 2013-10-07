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
  Impl(glib::Object<GVolume> const& volume, VolumeImp* parent)
    : parent_(parent)
    , volume_(volume)
  {
    signal_volume_changed_.Connect(volume_, "changed", [this] (GVolume*) {
      parent_->changed.emit();
    });

    signal_volume_removed_.Connect(volume_, "removed", [this] (GVolume*) {
      parent_->removed.emit();
    });
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

  void Eject()
  {
    if (!CanBeEjected())
      return;

    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(nullptr));

    g_volume_eject_with_operation(volume_, G_MOUNT_UNMOUNT_NONE, mount_op, cancellable_,
    [] (GObject* object, GAsyncResult* res, gpointer data) {
      if (g_volume_eject_with_operation_finish(G_VOLUME(object), res, nullptr))
        static_cast<Impl*>(data)->parent_->ejected.emit();
    }, this);
  }

  void Mount()
  {
    glib::Object<GMountOperation> mount_op(gtk_mount_operation_new(nullptr));

    g_volume_mount(volume_, G_MOUNT_MOUNT_NONE, mount_op, cancellable_,
    [] (GObject* object, GAsyncResult* res, gpointer data) {
      if (g_volume_mount_finish(G_VOLUME(object), res, nullptr))
        static_cast<Impl*>(data)->parent_->mounted.emit();
    }, this);
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

    g_drive_stop(drive, G_MOUNT_UNMOUNT_NONE, mount_op, cancellable_,
    [] (GObject* object, GAsyncResult* res, gpointer data) {
      if (g_drive_stop_finish(G_DRIVE(object), res, nullptr))
        static_cast<Impl*>(data)->parent_->stopped.emit();
    }, this);
  }

  void Unmount()
  {
    if (!IsMounted())
      return;

    glib::Object<GMount> mount(g_volume_get_mount(volume_));
    glib::Object<GMountOperation> op(gtk_mount_operation_new(nullptr));

    g_mount_unmount_with_operation(mount, G_MOUNT_UNMOUNT_NONE, op, cancellable_,
    [] (GObject* object, GAsyncResult* res, gpointer data) {
      if (g_mount_unmount_with_operation_finish(G_MOUNT(object), res, nullptr))
        static_cast<Impl*>(data)->parent_->unmounted.emit();
    }, this);
  }

  VolumeImp* parent_;
  glib::Cancellable cancellable_;
  glib::Object<GVolume> volume_;

  glib::Signal<void, GVolume*> signal_volume_changed_;
  glib::Signal<void, GVolume*> signal_volume_removed_;
};

//
// End private implementation
//

VolumeImp::VolumeImp(glib::Object<GVolume> const& volume)
  : pimpl(new Impl(volume, this))
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

std::string VolumeImp::GetUri() const
{
  return pimpl->GetUri();
}

bool VolumeImp::HasSiblings() const
{
  return pimpl->HasSiblings();
}

bool VolumeImp::IsMounted() const
{
  return pimpl->IsMounted();
}

void VolumeImp::Mount()
{
  pimpl->Mount();
}

void VolumeImp::Eject()
{
  pimpl->Eject();
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
