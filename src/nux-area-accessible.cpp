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
 * SECTION:nux-area-accessible
 * @Title: NuxAreaAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::Area
 * @see_also: nux::Area
 *
 * #NuxAreaAccessible implements the required ATK interfaces of
 * nux::Area, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 */

#include "nux-area-accessible.h"

#include "unitya11y.h"

/* GObject */
static void nux_area_accessible_class_init (NuxAreaAccessibleClass *klass);
static void nux_area_accessible_init       (NuxAreaAccessible *area_accessible);

/* AtkObject.h */
static void       nux_area_accessible_initialize     (AtkObject *accessible,
                                                      gpointer   data);
static AtkObject *nux_area_accessible_get_parent     (AtkObject *obj);
static AtkStateSet* nux_area_accessible_ref_state_set (AtkObject *obj);

/* AtkComponent.h */
static void atk_component_interface_init     (AtkComponentIface *iface);
static void nux_area_accessible_get_extents  (AtkComponent *component,
                                              gint         *x,
                                              gint         *y,
                                              gint         *width,
                                              gint         *height,
                                              AtkCoordType  coord_type);



G_DEFINE_TYPE_WITH_CODE (NuxAreaAccessible, nux_area_accessible,  NUX_TYPE_OBJECT_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT, atk_component_interface_init))

static void
nux_area_accessible_class_init (NuxAreaAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_area_accessible_initialize;
  atk_class->get_parent = nux_area_accessible_get_parent;
  atk_class->ref_state_set = nux_area_accessible_ref_state_set;
}

static void
nux_area_accessible_init (NuxAreaAccessible *area_accessible)
{
}

AtkObject*
nux_area_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::Area*>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_AREA_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_area_accessible_initialize (AtkObject *accessible,
                                gpointer data)
{
  ATK_OBJECT_CLASS (nux_area_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;
}

static AtkObject *
nux_area_accessible_get_parent (AtkObject *obj)
{
  nux::Object *nux_object = NULL;
  nux::Area *area = NULL;
  nux::Area *parent = NULL;

  g_return_val_if_fail (NUX_IS_AREA_ACCESSIBLE (obj), NULL);

  if (obj->accessible_parent)
    return obj->accessible_parent;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return NULL;

  area = dynamic_cast<nux::Area *>(nux_object);

  parent = area->GetParentObject ();

  return unity_a11y_get_accessible (parent);
}

/*
 * Checks if the parent actor, and his parent, etc is all visible
 * Used to check the showing state
 */
static gboolean
_check_all_parents_visible (nux::Area *area)
{
  nux::Area *iter_parent = NULL;
  gboolean result = TRUE;

  for (iter_parent = area; iter_parent;
       iter_parent = iter_parent->GetParentObject ())
    {
      if (!iter_parent->IsVisible())
        {
          result = FALSE;
          break;
        }
    }

  return result;
}

static AtkStateSet*
nux_area_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  nux::Object *nux_object = NULL;
  nux::Area *area = NULL;

  g_return_val_if_fail (NUX_IS_AREA_ACCESSIBLE (obj), NULL);

  state_set = ATK_OBJECT_CLASS (nux_area_accessible_parent_class)->ref_state_set (obj);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return state_set;

  area = dynamic_cast<nux::Area *>(nux_object);

  if (area->IsSensitive ())
    {
      atk_state_set_add_state (state_set, ATK_STATE_SENSITIVE);
      atk_state_set_add_state (state_set, ATK_STATE_ENABLED);
    }

  if (area->IsVisible ())
    {
      atk_state_set_add_state (state_set, ATK_STATE_VISIBLE);

      if (_check_all_parents_visible (area))
        atk_state_set_add_state (state_set, ATK_STATE_SHOWING);
    }

  return state_set;
}

/* AtkComponent implementation */
static void
atk_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->get_extents    = nux_area_accessible_get_extents;

  /* Focus management is done on NuxViewAccessible */
}

static void
nux_area_accessible_get_extents (AtkComponent *component,
                                 gint         *x,
                                 gint         *y,
                                 gint         *width,
                                 gint         *height,
                                 AtkCoordType coord_type)
{
  gint top_level_x = 0;
  gint top_level_y = 0;
  nux::Object *nux_object = NULL;
  nux::Area *area = NULL;
  nux::Geometry geometry;

  g_return_if_fail (NUX_IS_AREA_ACCESSIBLE (component));

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (component));

  if (nux_object == NULL) /* actor is defunct */
    return;

  area = dynamic_cast<nux::Area *>(nux_object);

  geometry = area->GetGeometry ();

  *width = geometry.GetWidth ();
  *height = geometry.GetWidth ();
  *x = geometry.x;
  *y = geometry.y;

  /* In the ATK_XY_WINDOW case
   *
   * http://library.gnome.org/devel/atk/stable/AtkUtil.html#AtkCoordType
   */

  if (coord_type == ATK_XY_SCREEN)
    {
      /* For the moment Unity is a full-screen app, so ATK_XY_SCREEN
         and ATK_XY_WINDOW are the same */
      *x += top_level_x;
      *y += top_level_y;
    }

  return;
}
