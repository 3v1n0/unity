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
#include "gmockvolume.h"

static void g_mock_volume_iface_init (GVolumeIface *iface);

G_DEFINE_TYPE_WITH_CODE (GMockVolume, g_mock_volume, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_VOLUME, g_mock_volume_iface_init))

static void
g_mock_volume_finalize (GObject *object)
{
  G_OBJECT_CLASS (g_mock_volume_parent_class)->finalize (object);
}

static void
g_mock_volume_dispose (GObject *object)
{
  GMockVolume *self = G_MOCK_VOLUME (object);

  self->can_eject = FALSE;

  if (self->name)
  {
    g_free(self->name);
    self->name = NULL;
  }

  if (self->icon)
  {
    g_object_unref(self->icon);
    self->icon = NULL;
  }

  if (self->uuid)
  {
    g_free(self->uuid);
    self->uuid = NULL;
  }

  if (self->label)
  {
    g_free(self->label);
    self->label = NULL;
  }

  if (self->mount)
  {
    g_object_unref(self->mount);
    self->mount = NULL;
  }

  G_OBJECT_CLASS (g_mock_volume_parent_class)->dispose (object);
}


static void
g_mock_volume_class_init (GMockVolumeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_mock_volume_finalize;
  gobject_class->dispose = g_mock_volume_dispose;
}

static void
g_mock_volume_init (GMockVolume *mock_volume)
{
  guint32 uuid = g_random_int();
  mock_volume->name = g_strdup_printf("MockVolume %u", uuid);
  mock_volume->icon = g_icon_new_for_string("", NULL);
  mock_volume->label = g_strdup("");
  mock_volume->uuid = g_strdup_printf("%u", uuid);
  mock_volume->mount = NULL;
  mock_volume->last_mount_had_mount_op = FALSE;
}

GMockVolume *
g_mock_volume_new ()
{
  GMockVolume *volume;

  volume = g_object_new (G_TYPE_MOCK_VOLUME, NULL);

  return volume;
}

void
g_mock_volume_set_name (GMockVolume *volume, const char* name)
{
  if (volume->name)
    g_free(volume->name);

  volume->name = g_strdup (name);
}

static char *
g_mock_volume_get_name (GVolume *volume)
{
  GMockVolume *self = G_MOCK_VOLUME (volume);
  return g_strdup (self->name);
}

void
g_mock_volume_set_icon (GMockVolume *volume, GIcon *icon)
{
  if (volume->icon)
    g_object_unref(volume->icon);

  volume->icon = icon;
}


static GIcon *
g_mock_volume_get_icon (GVolume *volume)
{
  GMockVolume *self = G_MOCK_VOLUME (volume);

  if (self->icon)
    return g_object_ref (self->icon);
  else
    return NULL;
}

void
g_mock_volume_set_uuid (GMockVolume *volume, const char* uuid)
{
  if (volume->uuid)
    g_free(volume->uuid);

  volume->uuid = g_strdup (uuid);
}

static char *
g_mock_volume_get_uuid (GVolume *volume)
{
  GMockVolume *self = G_MOCK_VOLUME (volume);
  return g_strdup (self->uuid);
}

void
g_mock_volume_set_label (GMockVolume *volume, const char* label)
{
  if (volume->label)
    g_free(volume->label);

  volume->label = g_strdup (label);
}

static GDrive *
g_mock_volume_get_drive (GVolume *volume)
{
  return NULL;
}

void
g_mock_volume_set_mount (GMockVolume *volume, GMount *mount)
{
  if (volume->mount)
    g_object_unref(volume->mount);

  volume->mount = mount;
}

static GMount *
g_mock_volume_get_mount (GVolume *volume)
{
  GMockVolume *self = G_MOCK_VOLUME (volume);
  if (self->mount)
    return g_object_ref (self->mount);
  else
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
  GMockVolume *self = G_MOCK_VOLUME (volume);
  return self->can_eject;
}

void
g_mock_volume_set_can_eject (GMockVolume* mock_volume, gboolean can_eject)
{
  mock_volume->can_eject = can_eject;
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
  GMockVolume *mock_volume = G_MOCK_VOLUME(volume);

  mock_volume->last_mount_had_mount_op = (mount_operation != NULL);
  g_mock_volume_set_mount(mock_volume, G_MOUNT(g_mock_mount_new()));

  callback(NULL,
           G_ASYNC_RESULT (g_simple_async_result_new (NULL, NULL, NULL, NULL)),
           user_data);
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
  
  callback(NULL,
           G_ASYNC_RESULT (g_simple_async_result_new (NULL, NULL, NULL, NULL)),
           user_data);}

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
  GMockVolume *self = G_MOCK_VOLUME (volume);

  if (!g_strcmp0 (kind, G_VOLUME_IDENTIFIER_KIND_UUID))
    return g_strdup (self->uuid);
  else if (!g_strcmp0 (kind, G_VOLUME_IDENTIFIER_KIND_LABEL))
    return g_strdup (self->label);

  return NULL;
}

static gchar **
g_mock_volume_enumerate_identifiers (GVolume *volume)
{
  return NULL;
}

gboolean
g_mock_volume_last_mount_had_mount_operation (GMockVolume* volume)
{
  return volume->last_mount_had_mount_op;
}

static void
g_mock_volume_iface_init (GVolumeIface *iface)
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
