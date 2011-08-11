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
 * SECTION:unity-places_view-accessible
 * @Title: UnityPlacesViewAccessible
 * @short_description: Implementation of the ATK interfaces for #PlacesView
 * @see_also: PlacesView
 *
 * #UnityPlacesViewAccessible implements the required ATK interfaces for
 * #PlacesView, ie: exposing the different PlacesViewIcon on the model as
 * #child of the object.
 *
 */

#include <glib/gi18n.h>

#include "unity-places-view-accessible.h"

#include "unitya11y.h"
#include "DashView.h"

using namespace unity::dash;

/* GObject */
static void unity_places_view_accessible_class_init(UnityPlacesViewAccessibleClass* klass);
static void unity_places_view_accessible_init(UnityPlacesViewAccessible* self);
static void unity_places_view_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_places_view_accessible_initialize(AtkObject* accessible,
                                                          gpointer   data);

G_DEFINE_TYPE(UnityPlacesViewAccessible, unity_places_view_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

#define UNITY_PLACES_VIEW_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_PLACES_VIEW_ACCESSIBLE,  \
                                UnityPlacesViewAccessiblePrivate))

struct _UnityPlacesViewAccessiblePrivate
{

};


static void
unity_places_view_accessible_class_init(UnityPlacesViewAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->finalize = unity_places_view_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_places_view_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnityPlacesViewAccessiblePrivate));
}

static void
unity_places_view_accessible_init(UnityPlacesViewAccessible* self)
{
  UnityPlacesViewAccessiblePrivate* priv =
    UNITY_PLACES_VIEW_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_places_view_accessible_finalize(GObject* object)
{
  G_OBJECT_CLASS(unity_places_view_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_places_view_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<DashView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_PLACES_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);
  atk_object_set_name(accessible, _("Places"));

  return accessible;
}

/* AtkObject.h */
static void
unity_places_view_accessible_initialize(AtkObject* accessible,
                                        gpointer data)
{
  ATK_OBJECT_CLASS(unity_places_view_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

