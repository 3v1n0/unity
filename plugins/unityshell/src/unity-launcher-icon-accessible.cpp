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
 * SECTION:nux-launcher_icon-accessible
 * @Title: UnityLauncherIconAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::LauncherIcon
 * @see_also: nux::LauncherIcon
 *
 * #UnityLauncherIconAccessible implements the required ATK interfaces of
 * nux::LauncherIcon, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 * Implementation notes: on previous implementations we implemented
 * _get_parent using the LauncherIcon method GetLauncher. But this is
 * not the case in all the situations. When the user is interacting
 * with the Switcher, we consider that the parent of that LauncherIcon
 * is the Switcher.
 *
 * The parent is set with atk_object_set_parent as usual.
 *
 * As this object is used both on UnityLauncherAccessible and
 * UnitySwitcherAccessible, we have removed as much as possible any
 * reference to the Launcher, LauncherModel, SwitcherView or
 * SwitcherModel.
 *
 */

#include "unity-launcher-icon-accessible.h"
#include "unity-launcher-accessible.h"
#include "Launcher.h"
#include "LauncherIcon.h"

#include "unitya11y.h"

using namespace unity::launcher;

/* GObject */
static void unity_launcher_icon_accessible_class_init(UnityLauncherIconAccessibleClass* klass);
static void unity_launcher_icon_accessible_init(UnityLauncherIconAccessible* launcher_icon_accessible);
static void unity_launcher_icon_accessible_dispose(GObject* object);


/* AtkObject.h */
static void          unity_launcher_icon_accessible_initialize(AtkObject* accessible,
                                                               gpointer   data);
static AtkStateSet*  unity_launcher_icon_accessible_ref_state_set(AtkObject* obj);
static const gchar* unity_launcher_icon_accessible_get_name(AtkObject* obj);
// static AtkObject*    unity_launcher_icon_accessible_get_parent(AtkObject* obj);
static gint          unity_launcher_icon_accessible_get_index_in_parent(AtkObject* obj);

/* AtkComponent.h */
static void     atk_component_interface_init(AtkComponentIface* iface);
static guint    unity_launcher_icon_accessible_add_focus_handler(AtkComponent* component,
                                                                 AtkFocusHandler handler);
static void     unity_launcher_icon_accessible_remove_focus_handler(AtkComponent* component,
                                                                    guint handler_id);
static void     unity_launcher_icon_accessible_focus_handler(AtkObject* accessible,
                                                             gboolean focus_in);

/* AtkAction */
static void         atk_action_interface_init(AtkActionIface *iface);
static gboolean     unity_launcher_icon_accessible_do_action(AtkAction *action,
                                                              gint i);
static gint         unity_launcher_icon_accessible_get_n_actions(AtkAction *action);
static const gchar* unity_launcher_icon_accessible_get_name(AtkAction *action,
                                                            gint i);

/* private/utility methods*/
static void check_selected(UnityLauncherIconAccessible* self);
static void on_parent_selection_change_cb(AtkSelection* selection,
                                          gpointer data);
static void on_parent_focus_event_cb(AtkObject* object,
                                     gboolean in,
                                     gpointer data);

G_DEFINE_TYPE_WITH_CODE(UnityLauncherIconAccessible,
                        unity_launcher_icon_accessible,
                        NUX_TYPE_OBJECT_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT,
                                              atk_component_interface_init)
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_ACTION,
                                              atk_action_interface_init))

#define UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, \
                                UnityLauncherIconAccessiblePrivate))

struct _UnityLauncherIconAccessiblePrivate
{
  /* Cached values (used to avoid extra notifications) */
  gboolean selected;
  gboolean parent_focused;
  gboolean index_in_parent;

  guint on_parent_change_id;
  guint on_parent_selection_change_id;
  guint on_parent_focus_event_id;
};

static void
unity_launcher_icon_accessible_class_init(UnityLauncherIconAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_launcher_icon_accessible_dispose;

  /* AtkObject */
  atk_class->initialize = unity_launcher_icon_accessible_initialize;
  atk_class->get_name = unity_launcher_icon_accessible_get_name;
  atk_class->ref_state_set = unity_launcher_icon_accessible_ref_state_set;
  // atk_class->get_parent = unity_launcher_icon_accessible_get_parent;
  atk_class->get_index_in_parent = unity_launcher_icon_accessible_get_index_in_parent;

  g_type_class_add_private(gobject_class, sizeof(UnityLauncherIconAccessiblePrivate));
}

static void
unity_launcher_icon_accessible_init(UnityLauncherIconAccessible* launcher_icon_accessible)
{
  UnityLauncherIconAccessiblePrivate* priv =
    UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE(launcher_icon_accessible);

  launcher_icon_accessible->priv = priv;
}

static void
unity_launcher_icon_accessible_dispose(GObject* object)
{
  UnityLauncherIconAccessible* self = UNITY_LAUNCHER_ICON_ACCESSIBLE(object);
  AtkObject* parent = NULL;

  parent = atk_object_get_parent(ATK_OBJECT(object));

  if (parent != NULL)
  {
    if (self->priv->on_parent_selection_change_id != 0)
      g_signal_handler_disconnect(parent, self->priv->on_parent_selection_change_id);

    if (self->priv->on_parent_focus_event_id != 0)
      g_signal_handler_disconnect(parent, self->priv->on_parent_focus_event_id);
  }

  if (self->priv->on_parent_change_id != 0)
    g_signal_handler_disconnect(object, self->priv->on_parent_change_id);

  G_OBJECT_CLASS(unity_launcher_icon_accessible_parent_class)->dispose(object);
}


AtkObject*
unity_launcher_icon_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<LauncherIcon*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
on_parent_change_cb(gchar* property,
                    GValue* value,
                    gpointer data)
{
  AtkObject* parent = NULL;
  UnityLauncherIconAccessible* self = NULL;
  AtkStateSet* state_set = NULL;

  g_return_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(data));

  self = UNITY_LAUNCHER_ICON_ACCESSIBLE(data);
  parent = atk_object_get_parent(ATK_OBJECT(data));

  if (parent == NULL)
    return;

  self->priv->on_parent_selection_change_id =
    g_signal_connect(parent, "selection-changed",
                     G_CALLBACK(on_parent_selection_change_cb), self);

  self->priv->on_parent_focus_event_id =
    g_signal_connect(parent, "focus-event",
                     G_CALLBACK(on_parent_focus_event_cb), self);

  state_set = atk_object_ref_state_set(parent);
  if (atk_state_set_contains_state(state_set, ATK_STATE_FOCUSED))
  {
    self->priv->parent_focused = TRUE;
  }
  g_object_unref(state_set);
}

static void
unity_launcher_icon_accessible_initialize(AtkObject* accessible,
                                          gpointer data)
{
  UnityLauncherIconAccessible* self = NULL;

  ATK_OBJECT_CLASS(unity_launcher_icon_accessible_parent_class)->initialize(accessible, data);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE(accessible);

  accessible->role = ATK_ROLE_PUSH_BUTTON;

  atk_component_add_focus_handler(ATK_COMPONENT(accessible),
                                  unity_launcher_icon_accessible_focus_handler);

  /* we could do that by redefining ->set_parent */
  self->priv->on_parent_change_id =
    g_signal_connect(accessible, "notify::accessible-parent",
                     G_CALLBACK(on_parent_change_cb), self);
}


static const gchar*
unity_launcher_icon_accessible_get_name(AtkObject* obj)
{
  const gchar* name;

  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(obj), NULL);

  name = ATK_OBJECT_CLASS(unity_launcher_icon_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    LauncherIcon* icon = NULL;

    icon = dynamic_cast<LauncherIcon*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));

    if (icon == NULL) /* State is defunct */
      name = NULL;
    else
      name = icon->tooltip_text().c_str();
  }

  return name;
}

static AtkStateSet*
unity_launcher_icon_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  UnityLauncherIconAccessible* self = NULL;
  nux::Object* nux_object = NULL;
  LauncherIcon* icon = NULL;

  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(obj), NULL);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE(obj);

  state_set = ATK_OBJECT_CLASS(unity_launcher_icon_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  /* by default */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);

  icon = dynamic_cast<LauncherIcon*>(nux_object);

  if (icon->GetQuirk(LauncherIcon::Quirk::VISIBLE))
  {
    atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
    atk_state_set_add_state(state_set, ATK_STATE_SHOWING);
  }

  if (self->priv->selected)
  {
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
    atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
  }

  return state_set;
}

/* private methods */

/*
 * Checks if the current item is selected, and notify a selection
 * change if the selection has changed
 */
static void
check_selected(UnityLauncherIconAccessible* self)
{
  AtkObject* parent = NULL;
  gboolean found = FALSE;

  parent = atk_object_get_parent(ATK_OBJECT(self));
  if  (parent == NULL)
    return;

  found = atk_selection_is_child_selected(ATK_SELECTION(parent),
                                          self->priv->index_in_parent);

  if ((found) && (self->priv->parent_focused == FALSE))
    return;

  if (found != self->priv->selected)
  {
    gboolean return_val = FALSE;

    self->priv->selected = found;
    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_SELECTED,
                                   found);
    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_ACTIVE,
                                   found);

    g_signal_emit_by_name(self, "focus-event", self->priv->selected, &return_val);
    atk_focus_tracker_notify(ATK_OBJECT(self));
  }
}

static void
on_parent_selection_change_cb(AtkSelection* selection,
                              gpointer data)
{
  g_return_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(data));

  check_selected(UNITY_LAUNCHER_ICON_ACCESSIBLE(data));
}


static void
on_parent_focus_event_cb(AtkObject* object,
                         gboolean in,
                         gpointer data)
{
  UnityLauncherIconAccessible* self = NULL;

  g_return_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(data));

  self = UNITY_LAUNCHER_ICON_ACCESSIBLE(data);
  self->priv->parent_focused = in;

  /* we check the selection stuff again, to report the focus change
     now */
  check_selected(self);
}

/* AtkComponent.h */

static void
atk_component_interface_init(AtkComponentIface* iface)
{
  g_return_if_fail(iface != NULL);

  /* focus management */
  iface->add_focus_handler    = unity_launcher_icon_accessible_add_focus_handler;
  iface->remove_focus_handler = unity_launcher_icon_accessible_remove_focus_handler;

  /* FIXME: still missing the size and position methods. Remember that
   * this is not a nux::Area, and probably we would require to poke
   * the Launcher to get those positions
   */
}

/*
 * comment C&P from cally-actor:
 *
 * "These methods are basically taken from gail, as I don't see any
 * reason to modify it. It makes me wonder why it is really required
 * to be implemented in the toolkit"
 */

static guint
unity_launcher_icon_accessible_add_focus_handler(AtkComponent* component,
                                                 AtkFocusHandler handler)
{
  GSignalMatchType match_type;
  gulong ret;
  guint signal_id;

  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(component), 0);

  match_type = (GSignalMatchType)(G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC);
  signal_id = g_signal_lookup("focus-event", ATK_TYPE_OBJECT);

  ret = g_signal_handler_find(component, match_type, signal_id, 0, NULL,
                              (gpointer) handler, NULL);
  if (!ret)
  {
    return g_signal_connect_closure_by_id(component,
                                          signal_id, 0,
                                          g_cclosure_new(G_CALLBACK(handler), NULL,
                                                         (GClosureNotify) NULL),
                                          FALSE);
  }
  else
    return 0;
}

static void
unity_launcher_icon_accessible_remove_focus_handler(AtkComponent* component,
                                                    guint handler_id)
{
  g_return_if_fail(NUX_IS_VIEW_ACCESSIBLE(component));

  g_signal_handler_disconnect(component, handler_id);
}

static void
unity_launcher_icon_accessible_focus_handler(AtkObject* accessible,
                                             gboolean focus_in)
{
  g_return_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(accessible));

  atk_object_notify_state_change(accessible, ATK_STATE_FOCUSED, focus_in);
}

static gint
unity_launcher_icon_accessible_get_index_in_parent(AtkObject* obj)
{
  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(obj), -1);

  return UNITY_LAUNCHER_ICON_ACCESSIBLE(obj)->priv->index_in_parent;
}

/* AtkAction */
static void
atk_action_interface_init(AtkActionIface *iface)
{
  iface->do_action = unity_launcher_icon_accessible_do_action;
  iface->get_n_actions = unity_launcher_icon_accessible_get_n_actions;
  iface->get_name = unity_launcher_icon_accessible_get_name;
}

static gboolean
unity_launcher_icon_accessible_do_action(AtkAction *action,
                                         gint i)
{
  LauncherIcon* icon = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(action), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(action));
  if (nux_object == NULL)
    return FALSE;

  icon = dynamic_cast<LauncherIcon*>(nux_object);

  icon->Activate(ActionArg(ActionArg::Source::LAUNCHER, 0));

  return TRUE;
}

static gint
unity_launcher_icon_accessible_get_n_actions(AtkAction *action)
{
  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(action), 0);

  return 1;
}

static const gchar*
unity_launcher_icon_accessible_get_name(AtkAction *action,
                                        gint i)
{
  g_return_val_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(action), NULL);
  g_return_val_if_fail(i == 0, NULL);

  return "activate";
}

/* Public */

void
unity_launcher_icon_accessible_set_index(UnityLauncherIconAccessible* self,
                                         gint index)
{
  g_return_if_fail(UNITY_IS_LAUNCHER_ICON_ACCESSIBLE(self));

  self->priv->index_in_parent = index;
}
