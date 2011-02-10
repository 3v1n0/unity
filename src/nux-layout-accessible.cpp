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
 * SECTION:nux-layout-accessible
 * @Title: NuxLayoutAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::Layout
 * @see_also: nux::Layout
 *
 * #NuxLayoutAccessible implements the required ATK interfaces of
 * nux::Layout, implementing the container related methods on
 * AtkObject, in order to expose his objects
 *
 */

#include "nux-layout-accessible.h"

#include "unitya11y.h"

/* GObject */
static void nux_layout_accessible_class_init (NuxLayoutAccessibleClass *klass);
static void nux_layout_accessible_init       (NuxLayoutAccessible *layout_accessible);

/* AtkObject.h */
static void       nux_layout_accessible_initialize     (AtkObject *accessible,
                                                        gpointer   data);
static gint       nux_layout_accessible_get_n_children (AtkObject *obj);
static AtkObject *nux_layout_accessible_ref_child      (AtkObject *obj,
                                                        gint i);


#define NUX_LAYOUT_ACCESSIBLE_GET_PRIVATE(obj)                          \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_LAYOUT_ACCESSIBLE, NuxLayoutAccessiblePrivate))

G_DEFINE_TYPE (NuxLayoutAccessible, nux_layout_accessible,  NUX_TYPE_AREA_ACCESSIBLE)

struct _NuxLayoutAccessiblePrivate
{
};

static void
nux_layout_accessible_class_init (NuxLayoutAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_layout_accessible_initialize;
  atk_class->ref_child = nux_layout_accessible_ref_child;
  atk_class->get_n_children = nux_layout_accessible_get_n_children;

  g_type_class_add_private (gobject_class, sizeof (NuxLayoutAccessiblePrivate));
}

static void
nux_layout_accessible_init (NuxLayoutAccessible *layout_accessible)
{
  layout_accessible->priv = NUX_LAYOUT_ACCESSIBLE_GET_PRIVATE (layout_accessible);
}

AtkObject*
nux_layout_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::Layout *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_LAYOUT_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_layout_accessible_initialize (AtkObject *accessible,
                                  gpointer data)
{
  ATK_OBJECT_CLASS (nux_layout_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

static gint
nux_layout_accessible_get_n_children (AtkObject *obj)
{
  nux::Object *nux_object = NULL;
  nux::Layout *layout = NULL;
  std::list<nux::Area *> element_list;

  g_return_val_if_fail (NUX_IS_LAYOUT_ACCESSIBLE (obj), 0);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (!nux_object) /* state is defunct */
    return 0;

  layout = dynamic_cast<nux::Layout *>(nux_object);

  element_list = layout->GetChildren ();

  return element_list.size ();
}

static AtkObject *
nux_layout_accessible_ref_child (AtkObject *obj,
                                 gint i)
{
  nux::Object *nux_object = NULL;
  nux::Object *child = NULL;
  AtkObject *child_accessible = NULL;
  nux::Layout *layout = NULL;
  std::list<nux::Area *> element_list;
  gint num = 0;
  std::list<nux::Area *>::iterator it;

  g_return_val_if_fail (NUX_IS_LAYOUT_ACCESSIBLE (obj), 0);
  num = atk_object_get_n_accessible_children (obj);
  g_return_val_if_fail ((i < num)&&(i >= 0), NULL);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (!nux_object) /* state is defunct */
    return 0;

  layout = dynamic_cast<nux::Layout *>(nux_object);

  element_list = layout->GetChildren ();

  it = element_list.begin ();
  std::advance (it, i);

  child = dynamic_cast<nux::Object *>(*it);
  child_accessible = unity_a11y_get_accessible (child);

  g_object_ref (child_accessible);

  return child_accessible;
}
