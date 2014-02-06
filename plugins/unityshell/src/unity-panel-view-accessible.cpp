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
 * Authored by: Rodrigo Moya <rodrigo.moya@canonical.com>
 */

/**
 * SECTION:unity-panel-view-accessible
 * @Title: UnityPanelViewAccessible
 * @short_description: Implementation of the ATK interfaces for #PanelView
 * @see_also: PanelView
 *
 * #UnityPanelViewAccessible implements the required ATK interfaces for
 * #PanelView, ie: exposing the different items contained in the panel
 * as children.
 *
 */

#include <glib/gi18n-lib.h>
#include <Nux/Nux.h>
#include "PanelView.h"
#include "unity-panel-view-accessible.h"

#include "unitya11y.h"

using namespace unity::panel;

/* GObject */
static void unity_panel_view_accessible_class_init(UnityPanelViewAccessibleClass* klass);
static void unity_panel_view_accessible_init(UnityPanelViewAccessible* self);

/* AtkObject */
static void         unity_panel_view_accessible_initialize(AtkObject* accessible, gpointer data);
static gint         unity_panel_view_accessible_get_n_children(AtkObject* accessible);
static AtkObject*   unity_panel_view_accessible_ref_child(AtkObject* accessible, gint i);

G_DEFINE_TYPE(UnityPanelViewAccessible, unity_panel_view_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

static void
unity_panel_view_accessible_class_init(UnityPanelViewAccessibleClass* klass)
{
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = unity_panel_view_accessible_initialize;
  atk_class->get_n_children = unity_panel_view_accessible_get_n_children;
  atk_class->ref_child = unity_panel_view_accessible_ref_child;
}

static void
unity_panel_view_accessible_init(UnityPanelViewAccessible* self)
{
}

AtkObject*
unity_panel_view_accessible_new(nux::Object* object)
{
  AtkObject* accessible;

  g_return_val_if_fail(dynamic_cast<PanelView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_PANEL_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

static void
unity_panel_view_accessible_initialize(AtkObject* accessible, gpointer data)
{
  ATK_OBJECT_CLASS(unity_panel_view_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

static gint
unity_panel_view_accessible_get_n_children(AtkObject* accessible)
{
  nux::Object* nux_object = NULL;
  gint rc = 0;

  g_return_val_if_fail(UNITY_IS_PANEL_VIEW_ACCESSIBLE(accessible), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  if (!nux_object) /* state is defunct */
    return 0;

  return rc;
}

static AtkObject*
unity_panel_view_accessible_ref_child(AtkObject* accessible, gint i)
{
  nux::Object* nux_object = NULL;
  AtkObject* child_accessible = NULL;

  g_return_val_if_fail(UNITY_IS_PANEL_VIEW_ACCESSIBLE(accessible), NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  if (!nux_object) /* state is defunct */
    return NULL;

  return child_accessible;
}
