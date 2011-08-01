/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#ifndef UNITY_LAUNCHER_ICON_ACCESSIBLE_H
#define UNITY_LAUNCHER_ICON_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-object-accessible.h"

G_BEGIN_DECLS

#define UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE            (unity_launcher_icon_accessible_get_type ())
#define UNITY_LAUNCHER_ICON_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, UnityLauncherIconAccessible))
#define UNITY_LAUNCHER_ICON_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, UnityLauncherIconAccessibleClass))
#define UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE))
#define UNITY_IS_LAUNCHER_ICON_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE))
#define UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, UnityLauncherIconAccessibleClass))

typedef struct _UnityLauncherIconAccessible        UnityLauncherIconAccessible;
typedef struct _UnityLauncherIconAccessibleClass   UnityLauncherIconAccessibleClass;
typedef struct _UnityLauncherIconAccessiblePrivate   UnityLauncherIconAccessiblePrivate;

struct _UnityLauncherIconAccessible
{
  NuxObjectAccessible parent;

  /*< private >*/
  UnityLauncherIconAccessiblePrivate* priv;
};

struct _UnityLauncherIconAccessibleClass
{
  NuxObjectAccessibleClass parent_class;
};

GType      unity_launcher_icon_accessible_get_type(void);
AtkObject* unity_launcher_icon_accessible_new(nux::Object* object);

void       unity_launcher_icon_accessible_set_index(UnityLauncherIconAccessible* self,
                                                    gint index);

G_END_DECLS

#endif /* __UNITY_LAUNCHER_ICON_ACCESSIBLE_H__ */
