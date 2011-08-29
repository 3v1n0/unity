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

G_BEGIN_DECLS

#define UNITY_TYPE_RESULT_ACCESSIBLE            (unity_result_accessible_get_type ())
#define UNITY_RESULT_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_RESULT_ACCESSIBLE, UnityResultAccessible))
#define UNITY_RESULT_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_RESULT_ACCESSIBLE, UnityResultAccessibleClass))
#define UNITY_IS_RESULT_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_RESULT_ACCESSIBLE))
#define UNITY_IS_RESULT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_RESULT_ACCESSIBLE))
#define UNITY_RESULT_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_RESULT_ACCESSIBLE, UnityResultAccessibleClass))

typedef struct _UnityResultAccessible        UnityResultAccessible;
typedef struct _UnityResultAccessibleClass   UnityResultAccessibleClass;
typedef struct _UnityResultAccessiblePrivate   UnityResultAccessiblePrivate;

struct _UnityResultAccessible
{
  AtkObject parent;

  /*< private >*/
  UnityResultAccessiblePrivate* priv;
};

struct _UnityResultAccessibleClass
{
  AtkObjectClass parent_class;
};

GType      unity_result_accessible_get_type(void);
AtkObject* unity_result_accessible_new();


G_END_DECLS

#endif /* __UNITY_RESULT_ACCESSIBLE_H__ */
