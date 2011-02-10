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

/**
 * SECTION:nux-object-accessible
 * @Title: NuxObjectAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::Object
 * @see_also: nux::Object
 *
 * #NuxObjectAccessible implements the required ATK interfaces of
 * nux::Object, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 */

#include "nux-object-accessible.h"

/* GObject */
static void nux_object_accessible_class_init (NuxObjectAccessibleClass *klass);
static void nux_object_accessible_init       (NuxObjectAccessible *object_accessible);

/* AtkObject.h */
static void       nux_object_accessible_initialize     (AtkObject *accessible,
                                                      gpointer   data);

#define NUX_OBJECT_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_OBJECT_ACCESSIBLE, NuxObjectAccessiblePrivate))

G_DEFINE_TYPE (NuxObjectAccessible, nux_object_accessible,  ATK_TYPE_OBJECT)

struct _NuxObjectAccessiblePrivate
{
  nux::Object *object;
};

static void
nux_object_accessible_class_init (NuxObjectAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_object_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (NuxObjectAccessiblePrivate));
}

static void
nux_object_accessible_init (NuxObjectAccessible *object_accessible)
{
  object_accessible->priv = NUX_OBJECT_ACCESSIBLE_GET_PRIVATE (object_accessible);

  object_accessible->priv->object = NULL;
}

AtkObject*
nux_object_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::Object *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_OBJECT_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_object_accessible_initialize (AtkObject *accessible,
                                gpointer data)
{
  NuxObjectAccessible *self = NULL;
  nux::Object *object = NULL;

  ATK_OBJECT_CLASS (nux_object_accessible_parent_class)->initialize (accessible, data);

  self = NUX_OBJECT_ACCESSIBLE (accessible);
  object = (nux::Object*)data;

  self->priv->object = object; /* FIXME: object destruction management (for
                                  the defunct state) */
  accessible->role = ATK_ROLE_UNKNOWN;
}

/**
 * nux_object_accessible_get_object:
 *
 * Returns the nux::Object this object is providing accessibility support.
 *
 * Note that there isn't a _set method. This is because setting that
 * should only be done during initilization, and it doesn't make sense
 * to change that during the life of the object.
 *
 */
nux::Object *
nux_object_accessible_get_object (NuxObjectAccessible *self)
{
  return self->priv->object;
}
