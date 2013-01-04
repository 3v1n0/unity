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
 * @Title: UnityQuicklistMenuAccessible
 * @short_description: Implementation of the ATK interfaces for #QuicklistView as a menu
 * @see_also: QuicklistView
 *
 * #UnityQuicklistAccessible implements the required ATK interfaces for
 * #QuicklistView, exposing himself as a menu.
 *
 * Note that this object is a QuicklistAccessible delegated object. If
 * you call unitya11y->get_accessible with a Quicklist it will return
 * a QuicklistAccessible. QuicklistMenuAccessible should only be
 * instantiated by QuicklistAccessible
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

#include "unity-quicklist-menu-accessible.h"

#include "unitya11y.h"
#include "Launcher.h" /*without this I get a error with the following include*/
#include "QuicklistView.h"

/* GObject */
static void unity_quicklist_menu_accessible_class_init(UnityQuicklistMenuAccessibleClass* klass);
static void unity_quicklist_menu_accessible_init(UnityQuicklistMenuAccessible* self);
static void unity_quicklist_menu_accessible_finalize(GObject* object);

/* AtkObject.h */
static void         unity_quicklist_menu_accessible_initialize(AtkObject* accessible,
                                                               gpointer   data);
static gint         unity_quicklist_menu_accessible_get_n_children(AtkObject* obj);
static AtkObject*   unity_quicklist_menu_accessible_ref_child(AtkObject* obj,
                                                              gint i);

/* AtkSelection */
static void       atk_selection_interface_init(AtkSelectionIface* iface);
static AtkObject* unity_quicklist_menu_accessible_ref_selection(AtkSelection* selection,
                                                                gint i);
static gint       unity_quicklist_menu_accessible_get_selection_count(AtkSelection* selection);
static gboolean   unity_quicklist_menu_accessible_is_child_selected(AtkSelection* selection,
                                                                    gint i);
/* private */
static void on_selection_change_cb(UnityQuicklistMenuAccessible* self);
static void on_parent_activate_change_cb(AtkObject* parent_window,
                                         UnityQuicklistMenuAccessible* self);
static void on_parent_change_cb(gchar* property,
                                GValue* value,
                                gpointer data);

G_DEFINE_TYPE_WITH_CODE(UnityQuicklistMenuAccessible,
                        unity_quicklist_menu_accessible,  NUX_TYPE_OBJECT_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_SELECTION, atk_selection_interface_init));


#define UNITY_QUICKLIST_MENU_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_QUICKLIST_MENU_ACCESSIBLE,  \
                                UnityQuicklistMenuAccessiblePrivate))

struct _UnityQuicklistMenuAccessiblePrivate
{
  sigc::connection on_selection_change_connection;
  guint on_parent_change_id;
  guint on_parent_activate_change_id;
};

using unity::QuicklistView;
using unity::QuicklistMenuItem;

static void
unity_quicklist_menu_accessible_class_init(UnityQuicklistMenuAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->finalize = unity_quicklist_menu_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_quicklist_menu_accessible_initialize;
  atk_class->get_n_children = unity_quicklist_menu_accessible_get_n_children;
  atk_class->ref_child = unity_quicklist_menu_accessible_ref_child;

  g_type_class_add_private(gobject_class, sizeof(UnityQuicklistMenuAccessiblePrivate));
}

static void
unity_quicklist_menu_accessible_init(UnityQuicklistMenuAccessible* self)
{
  UnityQuicklistMenuAccessiblePrivate* priv =
    UNITY_QUICKLIST_MENU_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_quicklist_menu_accessible_finalize(GObject* object)
{
  UnityQuicklistMenuAccessible* self = UNITY_QUICKLIST_MENU_ACCESSIBLE(object);

  self->priv->on_selection_change_connection.disconnect();

  if (self->priv->on_parent_change_id != 0)
    g_signal_handler_disconnect(object, self->priv->on_parent_change_id);

  G_OBJECT_CLASS(unity_quicklist_menu_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_quicklist_menu_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<QuicklistView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_QUICKLIST_MENU_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_quicklist_menu_accessible_initialize(AtkObject* accessible,
                                           gpointer data)
{
  nux::Object* nux_object = NULL;
  QuicklistView* quicklist = NULL;
  UnityQuicklistMenuAccessible* self = NULL;

  ATK_OBJECT_CLASS(unity_quicklist_menu_accessible_parent_class)->initialize(accessible, data);
  self = UNITY_QUICKLIST_MENU_ACCESSIBLE(accessible);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL) /* status defunct */
    return;

  atk_object_set_role(accessible, ATK_ROLE_MENU);
  atk_object_set_name(accessible, _("Quicklist"));

  self->priv->on_selection_change_connection =
    quicklist->selection_change.connect(sigc::bind(sigc::ptr_fun(on_selection_change_cb), self));

  self->priv->on_parent_change_id =
    g_signal_connect(accessible, "notify::accessible-parent",
                     G_CALLBACK(on_parent_change_cb), self);
}

static gint
unity_quicklist_menu_accessible_get_n_children(AtkObject* obj)
{
  QuicklistView* quicklist = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(obj), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL)
    return 0;

  return quicklist->GetNumItems();
}

static AtkObject*
unity_quicklist_menu_accessible_ref_child(AtkObject* obj,
                                          gint i)
{
  QuicklistView* quicklist = NULL;
  QuicklistMenuItem* child = NULL;
  AtkObject* child_accessible = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(obj), NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL)
    return NULL;

  child = quicklist->GetNthItems(i);
  child_accessible = unity_a11y_get_accessible(child);

  if (child_accessible != NULL)
  {
    AtkObject* parent = NULL;
    g_object_ref(child_accessible);
    parent = atk_object_get_parent(child_accessible);
    if (parent != obj)
      atk_object_set_parent(child_accessible, obj);
  }

  return child_accessible;
}

/* AtkSelection */
static void
atk_selection_interface_init(AtkSelectionIface* iface)
{
  iface->ref_selection = unity_quicklist_menu_accessible_ref_selection;
  iface->get_selection_count = unity_quicklist_menu_accessible_get_selection_count;
  iface->is_child_selected = unity_quicklist_menu_accessible_is_child_selected;

  /* NOTE: for the moment we don't provide the implementation for the
     "interactable" methods, it is, the methods that allow to change
     the selected icon. The QuicklistView doesn't provide that API, and
     right now  we are focusing on a normal user input.*/
}

static AtkObject*
unity_quicklist_menu_accessible_ref_selection(AtkSelection* selection,
                                              gint i)
{
  QuicklistView* quicklist = NULL;
  QuicklistMenuItem* child = NULL;
  AtkObject* child_accessible = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(selection), NULL);
  /* there can be only one item selected */
  g_return_val_if_fail(i == 0, NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL) /*state is defunct */
    return NULL;

  child = quicklist->GetSelectedMenuItem();
  child_accessible = unity_a11y_get_accessible(child);

  if (child_accessible != NULL)
    g_object_ref(child_accessible);

  return child_accessible;
}

static gint
unity_quicklist_menu_accessible_get_selection_count(AtkSelection* selection)
{
  QuicklistView* quicklist = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(selection), 0);

  /*
   * Looking at QuicklistView code, there is always one item selected,
   * anyway we check that there is at least one item
   */
  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL) /*state is defunct */
    return 0;

  if (quicklist->GetNumItems() > 0)
    return 1;
  else
    return 0;
}

static gboolean
unity_quicklist_menu_accessible_is_child_selected(AtkSelection* selection,
                                                  gint i)
{
  QuicklistView* quicklist = NULL;
  QuicklistMenuItem* selected = NULL;
  QuicklistMenuItem* ith_item = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(selection), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  quicklist = dynamic_cast<QuicklistView*>(nux_object);

  if (quicklist == NULL) /*state is defunct */
    return FALSE;

  selected = quicklist->GetSelectedMenuItem();
  ith_item = quicklist->GetNthItems(i);

  if (selected == ith_item)
    return TRUE;
  else
    return FALSE;
}

/* private */
static void
on_selection_change_cb(UnityQuicklistMenuAccessible* self)
{
  g_signal_emit_by_name(ATK_OBJECT(self), "selection-changed");
}

static void
on_parent_activate_change_cb(AtkObject* parent_window,
                             UnityQuicklistMenuAccessible* self)
{
  /* We consider that when our parent window is activated, the focus
     should be on the menu, specifically on one of the menu-item. So
     we emit a selection-change in order to notify that a selection
     was made */
  g_signal_emit_by_name(ATK_OBJECT(self), "selection-changed");
}


static void
on_parent_change_cb(gchar* property,
                    GValue* value,
                    gpointer data)
{
  AtkObject* parent = NULL;
  UnityQuicklistMenuAccessible* self = NULL;

  g_return_if_fail(UNITY_IS_QUICKLIST_MENU_ACCESSIBLE(data));
  self = UNITY_QUICKLIST_MENU_ACCESSIBLE(data);

  parent = atk_object_get_parent(ATK_OBJECT(self));

  if (parent == NULL)
    return;

  self->priv->on_parent_activate_change_id =
    g_signal_connect(parent, "activate",
                     G_CALLBACK(on_parent_activate_change_cb), self);
}

