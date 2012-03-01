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
 * SECTION:unity-quicklist-accessible
 * @Title: UnityQuicklistAccessible
 * @short_description: Implementation of the ATK interfaces for #QuicklistView
 * @see_also: QuicklistView
 *
 * #UnityQuicklistAccessible implements the required ATK interfaces for
 * #QuicklistView.
 *
 * IMPLEMENTATION NOTES:
 *
 *  The desired accessible object hierarchy is the following one:
 *    Role:menu
 *     Role:menu-item
 *     Role:menu-item.
 *
 *  But this quicklist is also a base window, so we can't set a role
 *  menu, and then keeping it sending window messages.
 *
 *  So a new object, with role menu will be added to the hierarchy:
 *  QuicklistMenu. It also hide the intermediate container objects.
 *
 * So we will have:
 *   Role:window (the quicklist itself)
 *     Role:menu (a dummy object having the role menu)
 *       Role:menuitem (From QuicklistView->GetChildren)
 *       Role:menuitem
 *
 */

#include <glib/gi18n.h>

#include "unity-quicklist-accessible.h"
#include "unity-quicklist-menu-accessible.h"

#include "unitya11y.h"
#include "Launcher.h" /*without this I get a error with the following include*/
#include "QuicklistView.h"

/* GObject */
static void unity_quicklist_accessible_class_init(UnityQuicklistAccessibleClass* klass);
static void unity_quicklist_accessible_init(UnityQuicklistAccessible* self);

/* AtkObject.h */
static void         unity_quicklist_accessible_initialize(AtkObject* accessible,
                                                          gpointer   data);
static gint         unity_quicklist_accessible_get_n_children(AtkObject* obj);
static AtkObject*   unity_quicklist_accessible_ref_child(AtkObject* obj,
                                                         gint i);

G_DEFINE_TYPE(UnityQuicklistAccessible, unity_quicklist_accessible,  NUX_TYPE_BASE_WINDOW_ACCESSIBLE);


#define UNITY_QUICKLIST_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_QUICKLIST_ACCESSIBLE,  \
                                UnityQuicklistAccessiblePrivate))

struct _UnityQuicklistAccessiblePrivate
{
  AtkObject* menu_accessible;
};

using unity::QuicklistView;

static void
unity_quicklist_accessible_class_init(UnityQuicklistAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = unity_quicklist_accessible_initialize;
  atk_class->get_n_children = unity_quicklist_accessible_get_n_children;
  atk_class->ref_child = unity_quicklist_accessible_ref_child;

  g_type_class_add_private(gobject_class, sizeof(UnityQuicklistAccessiblePrivate));
}

static void
unity_quicklist_accessible_init(UnityQuicklistAccessible* self)
{
  UnityQuicklistAccessiblePrivate* priv =
    UNITY_QUICKLIST_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  priv->menu_accessible = NULL;
}

AtkObject*
unity_quicklist_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<QuicklistView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_QUICKLIST_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_quicklist_accessible_initialize(AtkObject* accessible,
                                      gpointer data)
{
  nux::Object* nux_object = NULL;
  QuicklistView* quicklist = NULL;

  ATK_OBJECT_CLASS(unity_quicklist_accessible_parent_class)->initialize(accessible, data);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL) /* status defunct */
    return;
}

static gint
unity_quicklist_accessible_get_n_children(AtkObject* obj)
{
  QuicklistView* quicklist = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_ACCESSIBLE(obj), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL)
    return 0;
  else
    return 1;
}

static AtkObject*
unity_quicklist_accessible_ref_child(AtkObject* obj,
                                     gint i)
{
  UnityQuicklistAccessible* self = NULL;
  QuicklistView* quicklist = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_ACCESSIBLE(obj), NULL);
  self = UNITY_QUICKLIST_ACCESSIBLE(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);
  if (quicklist == NULL)
    return NULL;

  if (self->priv->menu_accessible == NULL)
  {
    self->priv->menu_accessible = unity_quicklist_menu_accessible_new(quicklist);
    atk_object_set_parent(self->priv->menu_accessible, ATK_OBJECT(self));
  }

  g_object_ref(self->priv->menu_accessible);

  return self->priv->menu_accessible;
}
