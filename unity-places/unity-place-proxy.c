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
/**
 * SECTION:unity-place_proxy
 * @short_description: Sub-class this to implement a place_proxys object in the
 * Unity homescreen.
 * @include: unity-place_proxy.h
 *
 * #UnityPlaceProxy implements the Unity PlaceProxy dbus protocol. It cannot be intiated
 * on it's own, instead it should be subclassed and the vtable functions
 * overridden to create your own Unity PlaceProxy.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "unity-place-proxy.h"

G_DEFINE_TYPE (UnityPlaceProxy, unity_place_proxy, G_TYPE_OBJECT)

#define UNITY_PLACE_PROXY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        UNITY_TYPE_PLACE_PROXY, UnityPlaceProxyPrivate))

struct _UnityPlaceProxyPrivate
{
  gchar *name;
  gchar *icon_name;
  gchar *tooltip;
};

/* Globals */
enum
{
  READY,

  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_ICON_NAME,
  PROP_TOOLTIP
};

static guint32 _proxy_signals[LAST_SIGNAL] = { 0 };

/* Forwards */

/* GObject Init */
static void
unity_place_proxy_finalize (GObject *place_proxy)
{
  UnityPlaceProxyPrivate *priv;

  priv = UNITY_PLACE_PROXY (place_proxy)->priv;

  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }
  if (priv->icon_name)
    {
      g_free (priv->icon_name);
      priv->icon_name = NULL;
    }
  if (priv->tooltip)
    {
      g_free (priv->tooltip);
      priv->tooltip = NULL;
    }

  G_OBJECT_CLASS (unity_place_proxy_parent_class)->finalize (place_proxy);
}

static void
unity_place_proxy_set_property (GObject      *object,
                          guint         id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  UnityPlaceProxy        *place_proxy = UNITY_PLACE_PROXY (object);
  UnityPlaceProxyPrivate *priv = place_proxy->priv;

  switch (id)
    {
    case PROP_NAME:
      if (priv->name)
        g_free (priv->name);
      priv->name = g_value_dup_string (value);
      g_object_notify (object, "name");
      break;

    case PROP_ICON_NAME:
      if (priv->icon_name)
        g_free (priv->icon_name);
      priv->icon_name = g_value_dup_string (value);
      g_object_notify (object, "icon-name");
      break;

    case PROP_TOOLTIP:
      if (priv->tooltip)
        g_free (priv->tooltip);
      priv->tooltip = g_value_dup_string (value);
      g_object_notify (object, "tooltip");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
unity_place_proxy_get_property (GObject    *object,
                          guint       id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  UnityPlaceProxy        *place_proxy = UNITY_PLACE_PROXY (object);
  UnityPlaceProxyPrivate *priv = place_proxy->priv;

  switch (id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, priv->icon_name);
      break;

    case PROP_TOOLTIP:
      g_value_set_string (value, priv->tooltip);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
      break;
    }
}

static void
unity_place_proxy_class_init (UnityPlaceProxyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  obj_class->finalize     = unity_place_proxy_finalize;
  obj_class->set_property = unity_place_proxy_set_property;
  obj_class->get_property = unity_place_proxy_get_property;

  /* Signals */
  /**
   * UnityPlaceProxy::ready:
   * @arg0: the #UnityPlaceProxy object
   *
   * Emitted when the place is ready for use (the initialization has been done
   * on the client side).
   **/
  _proxy_signals[READY] =
    g_signal_new ("ready",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (UnityPlaceProxyClass, ready),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /* Properties */
  pspec = g_param_spec_string ("name", "Name",
                               "Name of place",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (obj_class, PROP_NAME, pspec);

  pspec = g_param_spec_string ("icon-name", "Icon Name",
                               "Name of icon representing this place",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (obj_class, PROP_ICON_NAME, pspec);

  pspec = g_param_spec_string ("tooltip", "Tooltip",
                               "The tooltip to show for this place",
                               "",
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (obj_class, PROP_TOOLTIP, pspec);

  /* Add Private data */
  g_type_class_add_private (obj_class, sizeof (UnityPlaceProxyPrivate));
}

static void
unity_place_proxy_init (UnityPlaceProxy *place_proxy)
{
  UnityPlaceProxyPrivate *priv;

  priv = place_proxy->priv = UNITY_PLACE_PROXY_GET_PRIVATE (place_proxy);
}

/* Private methods */

/* Public methods */
