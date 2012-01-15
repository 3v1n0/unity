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
 * SECTION:unity-dash_view-accessible
 * @Title: UnityDashViewAccessible
 * @short_description: Implementation of the ATK interfaces for #DashView
 * @see_also: DashView
 *
 * #UnityDashViewAccessible implements the required ATK interfaces for
 * #DashView, ie: exposing the different DashViewIcon on the model as
 * #child of the object.
 *
 */

#include <glib/gi18n.h>

#include "unity-dash-view-accessible.h"

#include "unitya11y.h"
#include "DashView.h"

using namespace unity::dash;

/* GObject */
static void unity_dash_view_accessible_class_init(UnityDashViewAccessibleClass* klass);
static void unity_dash_view_accessible_init(UnityDashViewAccessible* self);
static void unity_dash_view_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_dash_view_accessible_initialize(AtkObject* accessible,
                                                        gpointer   data);

G_DEFINE_TYPE(UnityDashViewAccessible, unity_dash_view_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

#define UNITY_DASH_VIEW_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_DASH_VIEW_ACCESSIBLE,  \
                                UnityDashViewAccessiblePrivate))

struct _UnityDashViewAccessiblePrivate
{

};


static void
unity_dash_view_accessible_class_init(UnityDashViewAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->finalize = unity_dash_view_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_dash_view_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnityDashViewAccessiblePrivate));
}

static void
unity_dash_view_accessible_init(UnityDashViewAccessible* self)
{
  UnityDashViewAccessiblePrivate* priv =
    UNITY_DASH_VIEW_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_dash_view_accessible_finalize(GObject* object)
{
  G_OBJECT_CLASS(unity_dash_view_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_dash_view_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<DashView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_DASH_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);
  atk_object_set_name(accessible, _("Dash"));

  return accessible;
}

/* AtkObject.h */
static void
unity_dash_view_accessible_initialize(AtkObject* accessible,
                                      gpointer data)
{
  ATK_OBJECT_CLASS(unity_dash_view_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;
}

