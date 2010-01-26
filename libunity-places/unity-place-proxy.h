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

#ifndef _HAVE_UNITY_PLACE_PROXY_H
#define _HAVE_UNITY_PLACE_PROXY_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define UNITY_TYPE_PLACE_PROXY (unity_place_proxy_get_type ())

#define UNITY_PLACE_PROXY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        UNITY_TYPE_PLACE_PROXY, UnityPlaceProxy))

#define UNITY_PLACE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        UNITY_TYPE_PLACE_PROXY, UnityPlaceProxyClass))

#define UNITY_IS_PLACE_PROXY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        UNITY_TYPE_PLACE_PROXY))

#define UNITY_IS_PLACE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        UNITY_TYPE_PLACE_PROXY))

#define UNITY_PLACE_PROXY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        UNITY_TYPE_PLACE_PROXY, UnityPlaceProxyClass))

typedef struct _UnityPlaceProxy UnityPlaceProxy;
typedef struct _UnityPlaceProxyClass UnityPlaceProxyClass;
typedef struct _UnityPlaceProxyPrivate UnityPlaceProxyPrivate;

struct _UnityPlaceProxy
{
  GObject         parent;

  /*< private >*/
  UnityPlaceProxyPrivate   *priv;
};

struct _UnityPlaceProxyClass
{
  GObjectClass    parent_class;

  /*< public >*/

  /*< signals >*/
  void (*ready) (UnityPlaceProxy *proxy);

  /*< vtable >*/

  /*< private >*/
  void (*_unity_place_proxy_1) (void);
  void (*_unity_place_proxy_2) (void);
  void (*_unity_place_proxy_3) (void);
  void (*_unity_place_proxy_4) (void);
};

GType      unity_place_proxy_get_type         (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _HAVE_UNITY_PLACE_PROXY_H */
