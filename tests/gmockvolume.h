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

#ifndef UNITYSHELL_G_MOCK_VOLUME_H
#define UNITYSHELL_G_MOCK_VOLUME_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define G_TYPE_MOCK_VOLUME        (g_mock_volume_get_type ())
#define G_MOCK_VOLUME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_MOCK_VOLUME, GMockVolume))
#define G_MOCK_VOLUME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_MOCK_VOLUME, GMockVolumeClass))
#define G_IS_MOCK_VOLUME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_MOCK_VOLUME))
#define G_IS_MOCK_VOLUME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_MOCK_VOLUME))

typedef struct _GMockVolume GMockVolume;
typedef struct _GMockVolumeClass GMockVolumeClass;

struct _GMockVolume {
  GObject parent;

  gboolean can_eject;
  char *name;
  GIcon *icon;
  char *uuid;
  char *label;
  GMount *mount;
  gboolean last_mount_had_mount_op;
};

struct _GMockVolumeClass {
  GObjectClass parent_class;
};

GType         g_mock_volume_get_type  (void) G_GNUC_CONST;
GMockVolume * g_mock_volume_new       ();

void          g_mock_volume_set_can_eject (GMockVolume* volume, gboolean can_eject);
void          g_mock_volume_set_name  (GMockVolume *volume, const char *name);
void          g_mock_volume_set_icon  (GMockVolume *volume, GIcon *icon);
void          g_mock_volume_set_label  (GMockVolume *volume, const char *label);
void          g_mock_volume_set_uuid  (GMockVolume *volume, const char *uuid);
void          g_mock_volume_set_mount (GMockVolume *volume, GMount *mount);

gboolean      g_mock_volume_last_mount_had_mount_operation (GMockVolume *volume);

G_END_DECLS

#endif // UNITYSHELL_G_MOCK_VOLUME_H

