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

#include <glib.h>

#include "gmockmount.h"

static void g_mock_mount_iface_init (GMountIface *iface);

G_DEFINE_TYPE_WITH_CODE (GMockMount, g_mock_mount, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_MOUNT,
						g_mock_mount_iface_init))

static void
g_mock_mount_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_mock_mount_parent_class)->finalize (object);
}

static void
g_mock_mount_dispose (GObject *object)
{
  G_OBJECT_CLASS (g_mock_mount_parent_class)->dispose (object);
}


static void
g_mock_mount_class_init (GMockMountClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_mock_mount_finalize;
  gobject_class->dispose = g_mock_mount_dispose;
}

static void
g_mock_mount_init (GMockMount *mock_mount)
{}

GMockMount *
g_mock_mount_new ()
{
  GMockMount *mount;

  mount = g_object_new (G_TYPE_MOCK_MOUNT, NULL);

  return mount;
}

static GFile *
g_mock_mount_get_root (GMount *mount)
{
  return g_file_new_for_path (ROOT_FILE_PATH);
}

static GIcon *
g_mock_mount_get_icon (GMount *mount)
{
  return NULL;
}

static char *
g_mock_mount_get_uuid (GMount *mount)
{
  return g_strdup ("");
}

static char *
g_mock_mount_get_name (GMount *mount)
{
  return g_strdup ("");
}

static GDrive *
g_mock_mount_get_drive (GMount *mount)
{
  return NULL;
}

static GVolume *
g_mock_mount_get_volume (GMount *mount)
{
  return NULL;
}

static gboolean
g_mock_mount_can_unmount (GMount *mount)
{
  return TRUE;
}

static gboolean
g_mock_mount_can_eject (GMount *mount)
{
  return FALSE;
}

static void
g_mock_mount_unmount (GMount             *mount,
                      GMountUnmountFlags flags,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
}

static gboolean
g_mock_mount_unmount_finish (GMount       *mount,
                             GAsyncResult  *result,
                             GError       **error)
{
  return TRUE;
}

static void
g_mock_mount_eject (GMount             *mount,
                    GMountUnmountFlags flags,
                    GCancellable        *cancellable,
                    GAsyncReadyCallback  callback,
                    gpointer             user_data)
{
}

static gboolean
g_mock_mount_eject_finish (GMount       *mount,
                           GAsyncResult  *result,
                           GError       **error)
{
  return TRUE;
}


static void
g_mock_mount_iface_init (GMountIface *iface)
{
  iface->get_root = g_mock_mount_get_root;
  iface->get_name = g_mock_mount_get_name;
  iface->get_icon = g_mock_mount_get_icon;
  iface->get_uuid = g_mock_mount_get_uuid;
  iface->get_drive = g_mock_mount_get_drive;
  iface->get_volume = g_mock_mount_get_volume;
  iface->can_unmount = g_mock_mount_can_unmount;
  iface->can_eject = g_mock_mount_can_eject;
  iface->unmount = g_mock_mount_unmount;
  iface->unmount_finish = g_mock_mount_unmount_finish;
  iface->eject = g_mock_mount_eject;
  iface->eject_finish = g_mock_mount_eject_finish;
}
