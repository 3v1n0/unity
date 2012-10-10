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
 * SECTION:unity-switcher-accessible
 * @Title: UnitySwitcherAccessible
 * @short_description: Implementation of the ATK interfaces for #SwitcherView
 * @see_also: SwitcherView
 *
 * #UnitySwitcherAccessible implements the required ATK interfaces for
 * #SwitcherView, ie: exposing the different AbstractLauncherIcon on the
 * #model as child of the object.
 *
 */

#include <glib/gi18n.h>

#include "unity-switcher-accessible.h"
#include "unity-launcher-icon-accessible.h"

#include "unitya11y.h"
#include "SwitcherView.h"
#include "SwitcherModel.h"

using namespace unity::switcher;
using namespace unity::launcher;

/* GObject */
static void unity_switcher_accessible_class_init(UnitySwitcherAccessibleClass* klass);
static void unity_switcher_accessible_init(UnitySwitcherAccessible* self);
static void unity_switcher_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_switcher_accessible_initialize(AtkObject* accessible,
                                                       gpointer   data);
static gint       unity_switcher_accessible_get_n_children(AtkObject* obj);
static AtkObject* unity_switcher_accessible_ref_child(AtkObject* obj,
                                                      gint i);
static AtkStateSet* unity_switcher_accessible_ref_state_set(AtkObject* obj);

/* AtkSelection */
static void       atk_selection_interface_init(AtkSelectionIface* iface);
static AtkObject* unity_switcher_accessible_ref_selection(AtkSelection* selection,
                                                          gint i);
static gint       unity_switcher_accessible_get_selection_count(AtkSelection* selection);
static gboolean   unity_switcher_accessible_is_child_selected(AtkSelection* selection,
                                                              gint i);
/* NuxAreaAccessible */
static gboolean   unity_switcher_accessible_check_pending_notification(NuxAreaAccessible* self);

/* private */
static void       on_selection_changed_cb(AbstractLauncherIcon::Ptr const& icon,
                                          UnitySwitcherAccessible* switcher_accessible);
static void       create_children(UnitySwitcherAccessible* self);


G_DEFINE_TYPE_WITH_CODE(UnitySwitcherAccessible, unity_switcher_accessible,  NUX_TYPE_VIEW_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_SELECTION, atk_selection_interface_init))

#define UNITY_SWITCHER_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_SWITCHER_ACCESSIBLE,  \
                                UnitySwitcherAccessiblePrivate))

struct _UnitySwitcherAccessiblePrivate
{
  /* We maintain the children. Although the LauncherIcon are shared
   * between the Switcher and Launcher, in order to keep a hierarchy
   * coherence, we create a different accessible object  */
  GSList* children;

  sigc::connection on_selection_changed_connection;
};


static void
unity_switcher_accessible_class_init(UnitySwitcherAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);
  NuxAreaAccessibleClass* area_class = NUX_AREA_ACCESSIBLE_CLASS(klass);

  gobject_class->finalize = unity_switcher_accessible_finalize;

  /* AtkObject */
  atk_class->get_n_children = unity_switcher_accessible_get_n_children;
  atk_class->ref_child = unity_switcher_accessible_ref_child;
  atk_class->initialize = unity_switcher_accessible_initialize;
  atk_class->ref_state_set = unity_switcher_accessible_ref_state_set;

  /* NuxAreaAccessible */
  area_class->check_pending_notification = unity_switcher_accessible_check_pending_notification;

  g_type_class_add_private(gobject_class, sizeof(UnitySwitcherAccessiblePrivate));
}

static void
unity_switcher_accessible_init(UnitySwitcherAccessible* self)
{
  UnitySwitcherAccessiblePrivate* priv =
    UNITY_SWITCHER_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  self->priv->children = NULL;
}

static void
unity_switcher_accessible_finalize(GObject* object)
{
  UnitySwitcherAccessible* self = UNITY_SWITCHER_ACCESSIBLE(object);

  self->priv->on_selection_changed_connection.disconnect();

  if (self->priv->children)
  {
    g_slist_free_full(self->priv->children, g_object_unref);
    self->priv->children = NULL;
  }

  G_OBJECT_CLASS(unity_switcher_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_switcher_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<SwitcherView*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_SWITCHER_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);
  atk_object_set_name(accessible, _("Switcher"));

  return accessible;
}

/* AtkObject.h */
static void
unity_switcher_accessible_initialize(AtkObject* accessible,
                                     gpointer data)
{
  SwitcherView* switcher = NULL;
  nux::Object* nux_object = NULL;
  UnitySwitcherAccessible* self = NULL;
  SwitcherModel::Ptr model;

  ATK_OBJECT_CLASS(unity_switcher_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role(accessible, ATK_ROLE_TOOL_BAR);

  self = UNITY_SWITCHER_ACCESSIBLE(accessible);
  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  switcher = dynamic_cast<SwitcherView*>(nux_object);
  if (switcher == NULL)
    return;

  model = switcher->GetModel();

  if (model)
  {
    self->priv->on_selection_changed_connection  =
      model->selection_changed.connect(sigc::bind(sigc::ptr_fun(on_selection_changed_cb),
                                                  self));

    create_children(self);
  }

  /* To force being connected to the window::activate signal */
  nux_area_accessible_parent_window_active(NUX_AREA_ACCESSIBLE(self));
}

static gint
unity_switcher_accessible_get_n_children(AtkObject* obj)
{
  nux::Object* object = NULL;
  UnitySwitcherAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(obj), 0);
  self = UNITY_SWITCHER_ACCESSIBLE(obj);

  object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!object) /* state is defunct */
    return 0;

  return g_slist_length(self->priv->children);
}

static AtkObject*
unity_switcher_accessible_ref_child(AtkObject* obj,
                                    gint i)
{
  gint num = 0;
  nux::Object* nux_object = NULL;
  AtkObject* child_accessible = NULL;
  UnitySwitcherAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(obj), NULL);
  num = atk_object_get_n_accessible_children(obj);
  g_return_val_if_fail((i < num) && (i >= 0), NULL);
  self = UNITY_SWITCHER_ACCESSIBLE(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!nux_object) /* state is defunct */
    return 0;

  child_accessible = ATK_OBJECT(g_slist_nth_data(self->priv->children, i));

  g_object_ref(child_accessible);

  return child_accessible;
}

static AtkStateSet*
unity_switcher_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(obj), NULL);

  state_set =
    ATK_OBJECT_CLASS(unity_switcher_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  /* The Switcher is always focusable */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);

  /* The Switcher is always focused. Looking SwitcherController code,
   * SwitcherView is only created to be presented to the user */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);

  return state_set;
}

/* AtkSelection */
static void
atk_selection_interface_init(AtkSelectionIface* iface)
{
  iface->ref_selection = unity_switcher_accessible_ref_selection;
  iface->get_selection_count = unity_switcher_accessible_get_selection_count;
  iface->is_child_selected = unity_switcher_accessible_is_child_selected;

  /* NOTE: for the moment we don't provide the implementation for the
     "interactable" methods, it is, the methods that allow to change
     the selected icon. The Switcher doesn't provide that API, and
     right now  we are focusing on a normal user input.*/
  /* iface->add_selection = unity_switcher_accessible_add_selection; */
  /* iface->clear_selection = unity_switcher_accessible_clear_selection; */
  /* iface->remove_selection = unity_switcher_accessible_remove_selection; */

  /* This method will never be implemented, as select all the switcher
     icons makes no sense */
  /* iface->select_all = unity_switcher_accessible_select_all_selection; */
}

static AtkObject*
unity_switcher_accessible_ref_selection(AtkSelection* selection,
                                        gint i)
{
  SwitcherView* switcher = NULL;
  SwitcherModel::Ptr switcher_model;
  nux::Object* nux_object = NULL;
  gint selected_index = 0;
  AtkObject* accessible_selected = NULL;
  UnitySwitcherAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(selection), 0);
  /* there can be only just item selected */
  g_return_val_if_fail(i == 0, NULL);
  self = UNITY_SWITCHER_ACCESSIBLE(selection);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  if (!nux_object) /* state is defunct */
    return 0;

  switcher = dynamic_cast<SwitcherView*>(nux_object);

  switcher_model = switcher->GetModel();
  selected_index = switcher_model->SelectionIndex();

  accessible_selected = ATK_OBJECT(g_slist_nth_data(self->priv->children,
                                                    selected_index));

  if (accessible_selected != NULL)
    g_object_ref(accessible_selected);

  return accessible_selected;
}

static gint
unity_switcher_accessible_get_selection_count(AtkSelection* selection)
{
  SwitcherView* switcher = NULL;
  SwitcherModel::Ptr switcher_model;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(selection), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  if (!nux_object) /* state is defunct */
    return 0;

  switcher = dynamic_cast<SwitcherView*>(nux_object);
  switcher_model = switcher->GetModel();

  if (!switcher_model->Selection())
    return 0;
  else
    return 1;
}

static gboolean
unity_switcher_accessible_is_child_selected(AtkSelection* selection,
                                            gint i)
{
  SwitcherView* switcher = NULL;
  SwitcherModel::Ptr switcher_model;
  SwitcherModel::iterator it;
  nux::Object* nux_object = NULL;
  gint selected_index = 0;

  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(selection), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(selection));
  if (!nux_object) /* state is defunct */
    return 0;

  switcher = dynamic_cast<SwitcherView*>(nux_object);
  switcher_model = switcher->GetModel();
  selected_index = switcher_model->SelectionIndex();

  if (selected_index == i)
    return TRUE;
  else
    return FALSE;
}

/* NuxAreaAccessible */
static gboolean
unity_switcher_accessible_check_pending_notification(NuxAreaAccessible* self)
{
  g_return_val_if_fail(UNITY_IS_SWITCHER_ACCESSIBLE(self), FALSE);

  /* Overriding the method: the switcher doesn't get the key focus of
   * focus (Focusable) */
  /* From SwitcherController: it shows that the switcher only exists
   * to be shown to the user, so if the parent window gets actived, we
   * assume that the switcher will be automatically focused
   */
  atk_object_notify_state_change(ATK_OBJECT(self), ATK_STATE_FOCUSED, TRUE);
  g_signal_emit_by_name(self, "focus-event", TRUE, NULL);

  return TRUE;
}

/* private */
static void
on_selection_changed_cb(AbstractLauncherIcon::Ptr const& icon,
                        UnitySwitcherAccessible* switcher_accessible)
{
  g_signal_emit_by_name(ATK_OBJECT(switcher_accessible), "selection-changed");
}

static void
create_children(UnitySwitcherAccessible* self)
{
  gint index = 0;
  nux::Object* nux_object = NULL;
  SwitcherView* switcher = NULL;
  SwitcherModel::iterator it;
  AtkObject* child_accessible = NULL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (!nux_object) /* state is defunct */
    return;

  switcher = dynamic_cast<SwitcherView*>(nux_object);
  SwitcherModel::Ptr const& switcher_model = switcher->GetModel();

  if (!switcher_model)
    return;

  for (AbstractLauncherIcon::Ptr const& child : *switcher_model)
  {
    child_accessible = unity_launcher_icon_accessible_new(child.GetPointer());
    atk_object_set_parent(child_accessible, ATK_OBJECT(self));
    self->priv->children = g_slist_append(self->priv->children,
                                          child_accessible);
    unity_launcher_icon_accessible_set_index(UNITY_LAUNCHER_ICON_ACCESSIBLE(child_accessible),
                                             index++);
  }
}
