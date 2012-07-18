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

#include "gmockvolume.h"

static void g_mock_volume_volume_iface_init (GVolumeIface *iface);

#define g_mock_volume_get_type _g_mock_volume_get_type
G_DEFINE_TYPE_WITH_CODE (GMockVolume, g_mock_volume, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_VOLUME,
						g_mock_volume_volume_iface_init))

static void
g_mock_volume_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_mock_volume_parent_class)->finalize (object);
}

static void
g_mock_volume_class_init (GMockVolumeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_mock_volume_finalize;
}

static void
g_mock_volume_init (GMockVolume *mock_volume)
{
}

GMockVolume *
_g_mock_volume_new ()
{
  GMockVolume *volume;
  
  volume = g_object_new (G_TYPE_MOCK_VOLUME, NULL);

  return volume;
}

static char *
g_mock_volume_get_name (GVolume *volume)
{
  return g_strdup ("");
}

static GIcon *
g_mock_volume_get_icon (GVolume *volume)
{
  return g_icon_new_for_string("", NULL);
}

static char *
g_mock_volume_get_uuid (GVolume *volume)
{
  return NULL;
}

static GDrive *
g_mock_volume_get_drive (GVolume *volume)
{
  return NULL;
}

static GMount *
g_mock_volume_get_mount (GVolume *volume)
{
  return NULL;
}

static gboolean
g_mock_volume_can_mount (GVolume *volume)
{
  return TRUE;
}

static gboolean
g_mock_volume_can_eject (GVolume *volume)
{
  return FALSE;
}

static gboolean
g_mock_volume_should_automount (GVolume *volume)
{
  return TRUE;
}

static void
g_mock_volume_mount (GVolume            *volume,
                     GMountMountFlags    flags,
                     GMountOperation     *mount_operation,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
}

static gboolean
g_mock_volume_mount_finish (GVolume        *volume,
                            GAsyncResult  *result,
                            GError       **error)
{
  return TRUE;
}

static void
g_mock_volume_eject (GVolume             *volume,
                     GMountUnmountFlags   flags,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
}

static gboolean
g_mock_volume_eject_finish (GVolume       *volume,
                            GAsyncResult  *result,
                            GError       **error)
{
  return TRUE;
}

static gchar *
g_mock_volume_get_identifier (GVolume     *volume,
                              const gchar *kind)
{
  return NULL;
}

static gchar **
g_mock_volume_enumerate_identifiers (GVolume *volume)
{
  return NULL;
}

static void
g_mock_volume_volume_iface_init (GVolumeIface *iface)
{
  iface->get_name = g_mock_volume_get_name;
  iface->get_icon = g_mock_volume_get_icon;
  iface->get_uuid = g_mock_volume_get_uuid;
  iface->get_drive = g_mock_volume_get_drive;
  iface->get_mount = g_mock_volume_get_mount;
  iface->can_mount = g_mock_volume_can_mount;
  iface->can_eject = g_mock_volume_can_eject;
  iface->should_automount = g_mock_volume_should_automount;
  iface->mount_fn = g_mock_volume_mount;
  iface->mount_finish = g_mock_volume_mount_finish;
  iface->eject = g_mock_volume_eject;
  iface->eject_finish = g_mock_volume_eject_finish;
  iface->get_identifier = g_mock_volume_get_identifier;
  iface->enumerate_identifiers = g_mock_volume_enumerate_identifiers;
}

