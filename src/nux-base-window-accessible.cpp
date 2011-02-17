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
 * SECTION:nux-base_window-accessible
 * @Title: NuxBaseWindowAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::BaseWindow
 * @see_also: nux::BaseWindow
 *
 * Right now it is only here to expose the child of BaseWindow (the layout)
 *
 * #NuxBaseWindowAccessible implements the required ATK interfaces of
 * nux::BaseWindow, exposing as a child the BaseWindow layout
 *
 */

#include "nux-base-window-accessible.h"
#include "unitya11y.h"

#include "Nux/Area.h"
#include "Nux/Layout.h"

/* GObject */
static void nux_base_window_accessible_class_init (NuxBaseWindowAccessibleClass *klass);
static void nux_base_window_accessible_init       (NuxBaseWindowAccessible *base_window_accessible);

/* AtkObject.h */
static void       nux_base_window_accessible_initialize     (AtkObject *accessible,
                                                             gpointer   data);
static gint       nux_base_window_accessible_get_n_children (AtkObject *obj);
static AtkObject *nux_base_window_accessible_ref_child      (AtkObject *obj,
                                                             gint i);
static AtkObject *nux_base_window_accessible_get_parent     (AtkObject *obj);


G_DEFINE_TYPE (NuxBaseWindowAccessible, nux_base_window_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

static void
nux_base_window_accessible_class_init (NuxBaseWindowAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_base_window_accessible_initialize;
  atk_class->get_n_children = nux_base_window_accessible_get_n_children;
  atk_class->ref_child = nux_base_window_accessible_ref_child;
  atk_class->get_parent = nux_base_window_accessible_get_parent;
}

static void
nux_base_window_accessible_init (NuxBaseWindowAccessible *base_window_accessible)
{
}

AtkObject*
nux_base_window_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::BaseWindow*>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_BASE_WINDOW_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_base_window_accessible_initialize (AtkObject *accessible,
                                       gpointer data)
{
  ATK_OBJECT_CLASS (nux_base_window_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_WINDOW;
}

static gint
nux_base_window_accessible_get_n_children (AtkObject *obj)
{
  nux::Object *nux_object = NULL;
  nux::BaseWindow *base_window = NULL;
  nux::Layout *layout = NULL;

  g_return_val_if_fail (NUX_IS_BASE_WINDOW_ACCESSIBLE (obj), 0);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;
  base_window = dynamic_cast<nux::BaseWindow *>(nux_object);

  layout = base_window->GetLayout ();

  if (layout == NULL)
    return 0;
  else
    return 1;
}

static AtkObject *
nux_base_window_accessible_ref_child (AtkObject *obj,
                                      gint i)
{
  nux::Object *nux_object = NULL;
  nux::BaseWindow *base_window = NULL;
  nux::Layout *layout = NULL;
  AtkObject *layout_accessible = NULL;
  gint num = 0;

  g_return_val_if_fail (NUX_IS_BASE_WINDOW_ACCESSIBLE (obj), 0);
  num = atk_object_get_n_accessible_children (obj);
  g_return_val_if_fail ((i < num)&&(i >= 0), NULL);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;

  base_window = dynamic_cast<nux::BaseWindow *>(nux_object);

  layout = base_window->GetLayout ();

  layout_accessible = unity_a11y_get_accessible (layout);

  g_object_ref (layout_accessible);

  return layout_accessible;
}

static AtkObject
*nux_base_window_accessible_get_parent (AtkObject *obj)
{
  return NULL;
}
