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
 * SECTION:unity-quicklist_menu_item-accessible
 * @Title: UnityQuicklistMenuItemAccessible
 * @short_description: Implementation of the ATK interfaces for #QuicklistMenuItem
 * @see_also: QuicklistMenuItem
 *
 * #UnityQuicklistMenuItemAccessible implements the required ATK interfaces for
 * #QuicklistMenuItem, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>

#include "unity-quicklist-menu-item-accessible.h"

#include "unitya11y.h"
#include "QuicklistMenuItem.h"
#include "QuicklistMenuItemLabel.h"
#include "QuicklistMenuItemSeparator.h"

/* GObject */
static void unity_quicklist_menu_item_accessible_class_init(UnityQuicklistMenuItemAccessibleClass* klass);
static void unity_quicklist_menu_item_accessible_init(UnityQuicklistMenuItemAccessible* self);
static void unity_quicklist_menu_item_accessible_dispose(GObject* object);

/* AtkObject.h */
static void         unity_quicklist_menu_item_accessible_initialize(AtkObject* accessible,
                                                                    gpointer   data);
static const gchar* unity_quicklist_menu_item_accessible_get_name(AtkObject* obj);
static AtkStateSet* unity_quicklist_menu_item_accessible_ref_state_set(AtkObject* obj);

/* private */
static void on_parent_selection_change_cb(AtkSelection* selection,
                                          gpointer data);

G_DEFINE_TYPE(UnityQuicklistMenuItemAccessible, unity_quicklist_menu_item_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


#define UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE_GET_PRIVATE(obj)           \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_QUICKLIST_MENU_ITEM_ACCESSIBLE, \
                                UnityQuicklistMenuItemAccessiblePrivate))

struct _UnityQuicklistMenuItemAccessiblePrivate
{
  gboolean selected;

  guint on_parent_selection_change_id;
  guint on_parent_change_id;
};

using unity::QuicklistMenuItem;
using unity::QuicklistMenuItemLabel;
using unity::QuicklistMenuItemSeparator;

static void
unity_quicklist_menu_item_accessible_class_init(UnityQuicklistMenuItemAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_quicklist_menu_item_accessible_dispose;

  /* AtkObject */
  atk_class->get_name = unity_quicklist_menu_item_accessible_get_name;
  atk_class->initialize = unity_quicklist_menu_item_accessible_initialize;
  atk_class->ref_state_set = unity_quicklist_menu_item_accessible_ref_state_set;

  g_type_class_add_private(gobject_class, sizeof(UnityQuicklistMenuItemAccessiblePrivate));
}

static void
unity_quicklist_menu_item_accessible_init(UnityQuicklistMenuItemAccessible* self)
{
  UnityQuicklistMenuItemAccessiblePrivate* priv =
    UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_quicklist_menu_item_accessible_dispose(GObject* object)
{
  UnityQuicklistMenuItemAccessible* self = UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE(object);
  AtkObject* parent = NULL;

  parent = atk_object_get_parent(ATK_OBJECT(object));

  if (UNITY_IS_QUICKLIST_MENU_ITEM_ACCESSIBLE(parent))
  {
    if (self->priv->on_parent_selection_change_id != 0)
      g_signal_handler_disconnect(parent, self->priv->on_parent_selection_change_id);
  }

  if (self->priv->on_parent_change_id != 0)
    g_signal_handler_disconnect(object, self->priv->on_parent_change_id);

  G_OBJECT_CLASS(unity_quicklist_menu_item_accessible_parent_class)->dispose(object);
}

AtkObject*
unity_quicklist_menu_item_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<QuicklistMenuItem*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_QUICKLIST_MENU_ITEM_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static gboolean
menu_item_is_a_separator(QuicklistMenuItem* menu_item)
{
  QuicklistMenuItemSeparator* separator = NULL;

  separator = dynamic_cast<QuicklistMenuItemSeparator*>(menu_item);

  if (separator != NULL)
    return TRUE;
  else
    return FALSE;
}

static void
on_parent_change_cb(gchar* property,
                    GValue* value,
                    gpointer data)
{
  AtkObject* parent = NULL;
  UnityQuicklistMenuItemAccessible* self = NULL;

  g_return_if_fail(UNITY_IS_QUICKLIST_MENU_ITEM_ACCESSIBLE(data));
  self = UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE(data);

  parent = atk_object_get_parent(ATK_OBJECT(self));

  if (parent == NULL)
    return;

  self->priv->on_parent_selection_change_id =
    g_signal_connect(parent, "selection-changed",
                     G_CALLBACK(on_parent_selection_change_cb), self);
}

static void
unity_quicklist_menu_item_accessible_initialize(AtkObject* accessible,
                                                gpointer data)
{
  nux::Object* nux_object = NULL;
  QuicklistMenuItem* menu_item = NULL;
  UnityQuicklistMenuItemAccessible* self = NULL;

  ATK_OBJECT_CLASS(unity_quicklist_menu_item_accessible_parent_class)->initialize(accessible, data);
  self = UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE(accessible);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  menu_item = dynamic_cast<QuicklistMenuItem*>(nux_object);

  if (menu_item == NULL)
    return;

  if (menu_item_is_a_separator(menu_item))
    atk_object_set_role(accessible, ATK_ROLE_SEPARATOR);
  else
    atk_object_set_role(accessible, ATK_ROLE_MENU_ITEM);

  /* we could do that by redefining ->set_parent */
  self->priv->on_parent_change_id =
    g_signal_connect(accessible, "notify::accessible-parent",
                     G_CALLBACK(on_parent_change_cb), self);
}



static const gchar*
unity_quicklist_menu_item_accessible_get_name(AtkObject* obj)
{
  const gchar* name = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ITEM_ACCESSIBLE(obj), NULL);

  name = ATK_OBJECT_CLASS(unity_quicklist_menu_item_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    QuicklistMenuItem* menu_item = NULL;

    menu_item = dynamic_cast<QuicklistMenuItem*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));
    if (menu_item != NULL)
    {
      name = menu_item->GetPlainTextLabel().c_str();
    }
  }

  return name;
}

static AtkStateSet*
unity_quicklist_menu_item_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  UnityQuicklistMenuItemAccessible* self = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_QUICKLIST_MENU_ITEM_ACCESSIBLE(obj), NULL);
  self = UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE(obj);

  state_set = ATK_OBJECT_CLASS(unity_quicklist_menu_item_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  /* by default */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);

  if (self->priv->selected)
  {
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
    atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
  }
  else
  {
    /* we clean the states that could come from NuxAreaAccessible */
    atk_state_set_remove_state(state_set, ATK_STATE_FOCUSED);
  }

  return state_set;
}

/* private */
static void
check_selected(UnityQuicklistMenuItemAccessible* self)
{
  AtkObject* selected_item = NULL;
  AtkObject* parent = NULL;
  nux::Object* nux_object = NULL;
  gboolean found = FALSE;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (nux_object == NULL) /* state is defunct */
    return;

  parent = atk_object_get_parent(ATK_OBJECT(self));
  if (parent == NULL)
    return;

  selected_item = atk_selection_ref_selection(ATK_SELECTION(parent), 0);

  if (ATK_OBJECT(self) == selected_item)
    found = TRUE;

  if (found != self->priv->selected)
  {
    gboolean return_val = FALSE;

    self->priv->selected = found;
    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_FOCUSED,
                                   found);
    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_SELECTED,
                                   found);
    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_ACTIVE,
                                   found);

    g_signal_emit_by_name(self, "focus-event", self->priv->selected, &return_val);
  }
}

static void
on_parent_selection_change_cb(AtkSelection* selection,
                              gpointer data)
{
  g_return_if_fail(UNITY_IS_QUICKLIST_MENU_ITEM_ACCESSIBLE(data));

  check_selected(UNITY_QUICKLIST_MENU_ITEM_ACCESSIBLE(data));
}
