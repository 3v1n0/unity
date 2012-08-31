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

#ifndef UNITYSHELL_G_MOCK_MOUNT_H
#define UNITYSHELL_G_MOCK_MOUNT_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define ROOT_FILE_PATH "/some/directory/testfile"
#define ROOT_FILE_URI "file://" ROOT_FILE_PATH

#define G_TYPE_MOCK_MOUNT        (g_mock_mount_get_type ())
#define G_MOCK_MOUNT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_MOCK_MOUNT, GMockMount))
#define G_MOCK_MOUNT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_MOCK_MOUNT, GMockMountClass))
#define G_IS_MOCK_MOUNT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_MOCK_MOUNT))
#define G_IS_MOCK_MOUNT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_MOCK_MOUNT))

typedef struct _GMockMount GMockMount;
typedef struct _GMockMountClass GMockMountClass;

struct _GMockMount {
  GObject parent;
};

struct _GMockMountClass {
  GObjectClass parent_class;
};

GType         g_mock_mount_get_type  (void) G_GNUC_CONST;
GMockMount *  g_mock_mount_new       ();

G_END_DECLS

#endif // UNITYSHELL_G_MOCK_MOUNT_H
