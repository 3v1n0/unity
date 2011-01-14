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
 * SECTION:nux-view-accessible
 * @Title: NuxViewAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::View
 * @see_also: nux::View
 *
 * #NuxViewAccessible implements the required ATK interfaces of
 * nux::View
 *
 */

#include "nux-view-accessible.h"

/* GObject */
static void nux_view_accessible_class_init (NuxViewAccessibleClass *klass);
static void nux_view_accessible_init       (NuxViewAccessible *view_accessible);

/* AtkObject.h */
static void       nux_view_accessible_initialize     (AtkObject *accessible,
                                                      gpointer   data);

#define NUX_VIEW_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_VIEW_ACCESSIBLE, NuxViewAccessiblePrivate))

G_DEFINE_TYPE (NuxViewAccessible, nux_view_accessible,  NUX_TYPE_AREA_ACCESSIBLE)

struct _NuxViewAccessiblePrivate
{
};

static void
nux_view_accessible_class_init (NuxViewAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_view_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (NuxViewAccessiblePrivate));
}

static void
nux_view_accessible_init (NuxViewAccessible *view_accessible)
{
  view_accessible->priv = NUX_VIEW_ACCESSIBLE_GET_PRIVATE (view_accessible);
}

AtkObject*
nux_view_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::View *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_view_accessible_initialize (AtkObject *accessible,
                                gpointer data)
{
  ATK_OBJECT_CLASS (nux_view_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;
}
