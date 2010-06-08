/*
 * Copyright (C) 2010 Canonical, Ltd.
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
 * SECTION:unity-place
 * @short_description: Sub-class this to implement a places object in the
 * Unity homescreen.
 * @include: unity-place.h
 *
 * #UnityPlace implements the Unity Place dbus protocol. It cannot be intiated
 * on it's own, instead it should be subclassed and the vtable functions
 * overridden to create your own Unity Place.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "unity-place.h"
#include "unity-place-marshal.h"

G_DEFINE_TYPE (UnityPlace, unity_place, G_TYPE_OBJECT)

#define UNITY_PLACE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        UNITY_TYPE_PLACE, UnityPlacePrivate))

struct _UnityPlacePrivate
{
  gchar *name;
  gchar *icon_name;
  gchar *tooltip;

  gboolean is_active;
};

/* Globals */
enum
{
  VIEW_CHANGED,
  IS_ACTIVE,

  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_NAME,
  PROP_ICON_NAME,
  PROP_TOOLTIP
};

static guint32 _place_signals[LAST_SIGNAL] = { 0 };
static GQuark  unity_place_error_quark     = 0;

/* DBus Forwards (needed above the generated dbus header) */
static gboolean _unity_place_server_set_active (UnityPlace *place,
                                                gboolean    is_active,
                                                GError    **error);

#include "unity-place-server.h"

/* Forwards */

/* GObject Init */
static void
unity_place_finalize (GObject *place)
{
  UnityPlacePrivate *priv;

  priv = UNITY_PLACE (place)->priv;

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

  G_OBJECT_CLASS (unity_place_parent_class)->finalize (place);
}

static void
unity_place_set_property (GObject      *object,
                          guint         id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  UnityPlace        *place = UNITY_PLACE (object);
  UnityPlacePrivate *priv = place->priv;

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
unity_place_get_property (GObject    *object,
                          guint       id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  UnityPlace        *place = UNITY_PLACE (object);
  UnityPlacePrivate *priv = place->priv;

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
unity_place_class_init (UnityPlaceClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  obj_class->finalize     = unity_place_finalize;
  obj_class->set_property = unity_place_set_property;
  obj_class->get_property = unity_place_get_property;

  /* Signals */
  _place_signals[VIEW_CHANGED] =
    g_signal_new ("view-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  _unity_place_marshal_VOID__STRING_BOXED,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  DBUS_TYPE_G_STRING_STRING_HASHTABLE);

  _place_signals[IS_ACTIVE] =
    g_signal_new ("is-active",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

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

  /* Register as a DBus object */
  dbus_g_object_type_install_info (UNITY_TYPE_PLACE,
                                   &dbus_glib__unity_place_server_object_info);

  /* Add Private data */
  g_type_class_add_private (obj_class, sizeof (UnityPlacePrivate));
}

static void
unity_place_init (UnityPlace *place)
{
  UnityPlacePrivate      *priv;

  priv = place->priv = UNITY_PLACE_GET_PRIVATE (place);

  if (!unity_place_error_quark)
    unity_place_error_quark = g_quark_from_string ("unity-place-error");
}

/* Private methods */

static gboolean
_unity_place_server_set_active (UnityPlace *self,
                                gboolean    is_active,
                                GError    **error)
{
  g_return_val_if_fail (UNITY_IS_PLACE (self), FALSE);

  self->priv->is_active = is_active;

  g_signal_emit (self, _place_signals[IS_ACTIVE], 0, is_active);

  return TRUE;
}
