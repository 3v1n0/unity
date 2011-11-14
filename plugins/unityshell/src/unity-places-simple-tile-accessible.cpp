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
 * SECTION:unity-places_simple_tile-accessible
 * @Title: UnityPlacesSimpleTileAccessible
 * @short_description: Implementation of the ATK interfaces for #PlacesSimpleTile
 * @see_also: PlacesSimpleTile
 *
 * #UnityPlacesSimpleTileAccessible implements the required ATK interfaces for
 * #PlacesSimpleTile, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>

#include "unity-places-simple-tile-accessible.h"

#include "unitya11y.h"
#include "PlacesSimpleTile.h"

/* GObject */
static void unity_places_simple_tile_accessible_class_init(UnityPlacesSimpleTileAccessibleClass* klass);
static void unity_places_simple_tile_accessible_init(UnityPlacesSimpleTileAccessible* self);

/* AtkObject.h */
static void         unity_places_simple_tile_accessible_initialize(AtkObject* accessible,
                                                                   gpointer   data);
static const gchar* unity_places_simple_tile_accessible_get_name(AtkObject* obj);

G_DEFINE_TYPE(UnityPlacesSimpleTileAccessible, unity_places_simple_tile_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


#define UNITY_PLACES_SIMPLE_TILE_ACCESSIBLE_GET_PRIVATE(obj)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_PLACES_SIMPLE_TILE_ACCESSIBLE, \
                                UnityPlacesSimpleTileAccessiblePrivate))

struct _UnityPlacesSimpleTileAccessiblePrivate
{
  gchar* stripped_name;
};


static void
unity_places_simple_tile_accessible_class_init(UnityPlacesSimpleTileAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->get_name = unity_places_simple_tile_accessible_get_name;
  atk_class->initialize = unity_places_simple_tile_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnityPlacesSimpleTileAccessiblePrivate));
}

static void
unity_places_simple_tile_accessible_init(UnityPlacesSimpleTileAccessible* self)
{
  UnityPlacesSimpleTileAccessiblePrivate* priv =
    UNITY_PLACES_SIMPLE_TILE_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  priv->stripped_name = NULL;
}

AtkObject*
unity_places_simple_tile_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<unity::PlacesSimpleTile*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_PLACES_SIMPLE_TILE_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_places_simple_tile_accessible_initialize(AtkObject* accessible,
                                               gpointer data)
{
  ATK_OBJECT_CLASS(unity_places_simple_tile_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role(accessible, ATK_ROLE_PUSH_BUTTON);
}



static const gchar*
unity_places_simple_tile_accessible_get_name(AtkObject* obj)
{
  const gchar* name;
  UnityPlacesSimpleTileAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_PLACES_SIMPLE_TILE_ACCESSIBLE(obj), NULL);
  self = UNITY_PLACES_SIMPLE_TILE_ACCESSIBLE(obj);

  name = ATK_OBJECT_CLASS(unity_places_simple_tile_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    unity::PlacesSimpleTile* tile = NULL;

    if (self->priv->stripped_name != NULL)
    {
      g_free(self->priv->stripped_name);
      self->priv->stripped_name = NULL;
    }

    tile = dynamic_cast<unity::PlacesSimpleTile*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));
    if (tile != NULL)
    {
      name = tile->GetLabel();
      pango_parse_markup(name, -1, 0, NULL,
                         &self->priv->stripped_name,
                         NULL, NULL);
    }
  }

  return self->priv->stripped_name;
}
