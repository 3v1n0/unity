/*
 * Copyright (C) 2009 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef _HAVE_UNITY_PLACE_H
#define _HAVE_UNITY_PLACE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define UNITY_TYPE_PLACE (unity_place_get_type ())

#define UNITY_PLACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        UNITY_TYPE_PLACE, UnityPlace))

#define UNITY_PLACE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        UNITY_TYPE_PLACE, UnityPlaceClass))

#define UNITY_IS_PLACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        UNITY_TYPE_PLACE))

#define UNITY_IS_PLACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        UNITY_TYPE_PLACE))

#define UNITY_PLACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        UNITY_TYPE_PLACE, UnityPlaceClass))

typedef struct _UnityPlace UnityPlace;
typedef struct _UnityPlaceClass UnityPlaceClass;
typedef struct _UnityPlacePrivate UnityPlacePrivate;

struct _UnityPlace
{
  GObject         parent;

  /*< private >*/
  UnityPlacePrivate   *priv;
};

struct _UnityPlaceClass
{
  GObjectClass    parent_class;

  /*< public >*/

  /*< signals >*/
  void (*is_active) (UnityPlace *place, gboolean is_active);

  /*< vtable >*/

  /*< private >*/
  void (*_unity_place_1) (void);
  void (*_unity_place_2) (void);
  void (*_unity_place_3) (void);
  void (*_unity_place_4) (void);
};

GType  unity_place_get_type         (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _HAVE_UNITY_PLACE_H */
