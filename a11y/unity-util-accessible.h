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

#ifndef UNITY_UTIL_ACCESSIBLE_H
#define UNITY_UTIL_ACCESSIBLE_H

#include <atk/atk.h>

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

G_BEGIN_DECLS

#define UNITY_TYPE_UTIL_ACCESSIBLE            (unity_util_accessible_get_type ())
#define UNITY_UTIL_ACCESSIBLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNITY_TYPE_UTIL_ACCESSIBLE, UnityUtilAccessible))
#define UNITY_UTIL_ACCESSIBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNITY_TYPE_UTIL_ACCESSIBLE, UnityUtilAccessibleClass))
#define UNITY_IS_UTIL_ACCESSIBLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNITY_TYPE_UTIL_ACCESSIBLE))
#define UNITY_IS_UTIL_ACCESSIBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNITY_TYPE_UTIL_ACCESSIBLE))
#define UNITY_UTIL_ACCESSIBLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), UNITY_TYPE_UTIL_ACCESSIBLE, UnityUtilAccessibleClass))

typedef struct _UnityUtilAccessible        UnityUtilAccessible;
typedef struct _UnityUtilAccessibleClass   UnityUtilAccessibleClass;
typedef struct _UnityUtilAccessiblePrivate UnityUtilAccessiblePrivate;

struct _UnityUtilAccessible
{
  AtkUtil parent;

  /* < private > */
  UnityUtilAccessiblePrivate* priv;
};

struct _UnityUtilAccessibleClass
{
  AtkUtilClass parent_class;
};

GType unity_util_accessible_get_type(void);

void        unity_util_accessible_set_window_thread(nux::WindowThread* wt);
void        explore_children(AtkObject* obj);

G_END_DECLS

#endif /* UNITY_UTIL_ACCESSIBLE_H */
