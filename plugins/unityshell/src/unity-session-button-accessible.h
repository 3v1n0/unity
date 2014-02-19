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

#ifndef UNITY_SESSION_BUTTON_ACCESSIBLE_H
#define UNITY_SESSION_BUTTON_ACCESSIBLE_H

#include <atk/atk.h>

#include "nux-object-accessible.h"

G_BEGIN_DECLS

#define UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE            (unity_session_button_accessible_get_type ())
#define UNITY_SESSION_BUTTON_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE, UnitySessionButtonAccessible))
#define UNITY_SESSION_BUTTON_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE, UnitySessionButtonAccessibleClass))
#define UNITY_IS_SESSION_BUTTON_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE))
#define UNITY_IS_SESSION_BUTTON_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE))
#define UNITY_SESSION_BUTTON_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE, UnitySessionButtonAccessibleClass))

typedef struct _UnitySessionButtonAccessible        UnitySessionButtonAccessible;
typedef struct _UnitySessionButtonAccessibleClass   UnitySessionButtonAccessibleClass;
typedef struct _UnitySessionButtonAccessiblePrivate   UnitySessionButtonAccessiblePrivate;

struct _UnitySessionButtonAccessible
{
  NuxObjectAccessible parent;

  /*< private >*/
  UnitySessionButtonAccessiblePrivate* priv;
};

struct _UnitySessionButtonAccessibleClass
{
  NuxObjectAccessibleClass parent_class;
};

GType      unity_session_button_accessible_get_type(void);
AtkObject* unity_session_button_accessible_new(nux::Object* object);

G_END_DECLS

#endif /* __UNITY_SESSION_BUTTON_ACCESSIBLE_H__ */
