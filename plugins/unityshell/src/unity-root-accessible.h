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

#ifndef UNITY_ROOT_ACCESSIBLE_H
#define UNITY_ROOT_ACCESSIBLE_H

#include <atk/atk.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

G_BEGIN_DECLS

#define UNITY_TYPE_ROOT_ACCESSIBLE            (unity_root_accessible_get_type ())
#define UNITY_ROOT_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_ROOT_ACCESSIBLE, UnityRootAccessible))
#define UNITY_ROOT_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_ROOT_ACCESSIBLE, UnityRootAccessibleClass))
#define UNITY_IS_ROOT_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_ROOT_ACCESSIBLE))
#define UNITY_IS_ROOT_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_ROOT_ACCESSIBLE))
#define UNITY_ROOT_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_ROOT_ACCESSIBLE, UnityRootAccessibleClass))

typedef struct _UnityRootAccessible        UnityRootAccessible;
typedef struct _UnityRootAccessibleClass   UnityRootAccessibleClass;
typedef struct _UnityRootAccessiblePrivate UnityRootAccessiblePrivate;

struct _UnityRootAccessible
{
  AtkObject parent;

  /* < private > */
  UnityRootAccessiblePrivate* priv;
};

struct _UnityRootAccessibleClass
{
  AtkObjectClass parent_class;
};

GType      unity_root_accessible_get_type(void);
AtkObject* unity_root_accessible_new(void);

G_END_DECLS

#endif /* __UNITY_ROOT_ACCESSIBLE_H__ */
