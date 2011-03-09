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

#include "Nux/Area.h"
#include "Nux/Layout.h"

/* GObject */
static void nux_base_window_accessible_class_init (NuxBaseWindowAccessibleClass *klass);
static void nux_base_window_accessible_init       (NuxBaseWindowAccessible *base_window_accessible);

/* AtkObject.h */
static void       nux_base_window_accessible_initialize     (AtkObject *accessible,
                                                             gpointer   data);
static AtkObject *nux_base_window_accessible_get_parent     (AtkObject *obj);


G_DEFINE_TYPE (NuxBaseWindowAccessible, nux_base_window_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

static void
nux_base_window_accessible_class_init (NuxBaseWindowAccessibleClass *klass)
{
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_base_window_accessible_initialize;
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

  atk_object_set_name (accessible, "BaseWindow");

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

static AtkObject*
nux_base_window_accessible_get_parent (AtkObject *obj)
{
  return atk_get_root ();
}
