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
 */

#include "unity-launcher-icon-accessible.h"
#include "unity-launcher-accessible.h"
#include "LauncherIcon.h"

#include "unitya11y.h"

/* GObject */
static void unity_launcher_icon_accessible_class_init (UnityLauncherIconAccessibleClass *klass);
static void unity_launcher_icon_accessible_init       (UnityLauncherIconAccessible *launcher_icon_accessible);
static void unity_launcher_icon_accessible_dispose    (GObject *object);


/* AtkObject.h */
static void          unity_launcher_icon_accessible_initialize    (AtkObject *accessible,
                                                                   gpointer   data);
static AtkStateSet*  unity_launcher_icon_accessible_ref_state_set (AtkObject *obj);
static const gchar * unity_launcher_icon_accessible_get_name      (AtkObject *obj);
static AtkObject *   unity_launcher_icon_accessible_get_parent    (AtkObject *obj);
static gint          unity_launcher_icon_accessible_get_index_in_parent (AtkObject *obj);

/* AtkComponent.h */
static void     atk_component_interface_init                        (AtkComponentIface *iface);
static guint    unity_launcher_icon_accessible_add_focus_handler    (AtkComponent *component,
                                                                     AtkFocusHandler handler);
static void     unity_launcher_icon_accessible_remove_focus_handler (AtkComponent *component,
                                                                     guint handler_id);
static void     unity_launcher_icon_accessible_focus_handler        (AtkObject *accessible,
                                                                     gboolean focus_in);

/* private/utility methods*/
static void check_selected                (UnityLauncherIconAccessible *self);
static void on_parent_selection_change_cb (AtkSelection *selection,
                                           gpointer data);
static void on_parent_focus_event_cb      (AtkObject *object,
                                           gboolean in,
                                           gpointer data);

G_DEFINE_TYPE_WITH_CODE (UnityLauncherIconAccessible,
                         unity_launcher_icon_accessible,
                         NUX_TYPE_OBJECT_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                atk_component_interface_init))

#define UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, \
                                UnityLauncherIconAccessiblePrivate))

struct _UnityLauncherIconAccessiblePrivate
{
  /* Cached values (used to avoid extra notifications) */
  gboolean selected;
  gboolean parent_focused;
  gboolean index_in_parent;

  guint on_parent_selection_change_id;
  guint on_parent_focus_event_id;
};

static void
unity_launcher_icon_accessible_class_init (UnityLauncherIconAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  gobject_class->dispose = unity_launcher_icon_accessible_dispose;

  /* AtkObject */
  atk_class->initialize = unity_launcher_icon_accessible_initialize;
  atk_class->get_name = unity_launcher_icon_accessible_get_name;
  atk_class->ref_state_set = unity_launcher_icon_accessible_ref_state_set;
  atk_class->get_parent = unity_launcher_icon_accessible_get_parent;
  atk_class->get_index_in_parent = unity_launcher_icon_accessible_get_index_in_parent;

  g_type_class_add_private (gobject_class, sizeof (UnityLauncherIconAccessiblePrivate));
}

static void
unity_launcher_icon_accessible_init (UnityLauncherIconAccessible *launcher_icon_accessible)
{
  UnityLauncherIconAccessiblePrivate *priv =
    UNITY_LAUNCHER_ICON_ACCESSIBLE_GET_PRIVATE (launcher_icon_accessible);

  launcher_icon_accessible->priv = priv;
}

static void
unity_launcher_icon_accessible_dispose (GObject *object)
{
  UnityLauncherIconAccessible *self = UNITY_LAUNCHER_ICON_ACCESSIBLE (object);
  AtkObject *parent = NULL;

  parent = atk_object_get_parent (ATK_OBJECT (object));

  if (UNITY_IS_LAUNCHER_ACCESSIBLE (parent))
    {
      if (self->priv->on_parent_selection_change_id != 0)
        g_signal_handler_disconnect (parent, self->priv->on_parent_selection_change_id);

      if (self->priv->on_parent_focus_event_id != 0)
        g_signal_handler_disconnect (parent, self->priv->on_parent_focus_event_id);
    }

  G_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->dispose (object);
}


AtkObject*
unity_launcher_icon_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<LauncherIcon *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (UNITY_TYPE_LAUNCHER_ICON_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_launcher_icon_accessible_initialize (AtkObject *accessible,
                                           gpointer data)
{
  LauncherIcon *icon = NULL;
  Launcher *launcher = NULL;
  UnityLauncherAccessible *launcher_accessible = NULL;
  UnityLauncherIconAccessible *self = NULL;
  nux::Object *nux_object = NULL;

  ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->initialize (accessible, data);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE (accessible);

  accessible->role = ATK_ROLE_PUSH_BUTTON;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  icon = dynamic_cast<LauncherIcon *>(nux_object);
  launcher = icon->GetLauncher ();

  if (launcher == NULL)
    return;

  /* NOTE: we could also get the launcher_accessible by just calling
     atk_object_get_parent */
  launcher_accessible = UNITY_LAUNCHER_ACCESSIBLE (unity_a11y_get_accessible (launcher));

  self->priv->on_parent_selection_change_id =
    g_signal_connect (launcher_accessible, "selection-changed",
                      G_CALLBACK (on_parent_selection_change_cb), self);

  self->priv->on_parent_focus_event_id =
    g_signal_connect (launcher_accessible, "focus-event",
                      G_CALLBACK (on_parent_focus_event_cb), self);

  atk_component_add_focus_handler (ATK_COMPONENT (accessible),
                                   unity_launcher_icon_accessible_focus_handler);

  /* Check the cached selected state and notify the first selection.
   * Ie: it is required to ensure a first notification
   */
  check_selected (UNITY_LAUNCHER_ICON_ACCESSIBLE (accessible));
}


static const gchar *
unity_launcher_icon_accessible_get_name (AtkObject *obj)
{
  const gchar *name;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), NULL);

  name = ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->get_name (obj);
  if (name == NULL)
    {
      LauncherIcon *icon = NULL;

      icon = dynamic_cast<LauncherIcon *>(nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj)));

      if (icon == NULL) /* State is defunct */
        name = NULL;
      else
        name = icon->GetTooltipText().GetTCharPtr ();
    }

  return name;
}

static AtkStateSet*
unity_launcher_icon_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  UnityLauncherIconAccessible *self = NULL;
  nux::Object *nux_object = NULL;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), NULL);
  self = UNITY_LAUNCHER_ICON_ACCESSIBLE (obj);

  state_set = ATK_OBJECT_CLASS (unity_launcher_icon_accessible_parent_class)->ref_state_set (obj);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return state_set;

  if (self->priv->selected)
    {
      atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);
      atk_state_set_add_state (state_set, ATK_STATE_SELECTED);
      atk_state_set_add_state (state_set, ATK_STATE_ACTIVE);
    }

  return state_set;
}

static AtkObject *
unity_launcher_icon_accessible_get_parent (AtkObject *obj)
{
  nux::Object *nux_object = NULL;
  LauncherIcon *icon = NULL;
  Launcher *launcher = NULL;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), NULL);

  if (obj->accessible_parent)
    return obj->accessible_parent;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return NULL;

  icon = dynamic_cast<LauncherIcon *>(nux_object);
  launcher = icon->GetLauncher ();

  g_return_val_if_fail (dynamic_cast<Launcher *>(launcher), NULL);

  return unity_a11y_get_accessible (launcher);
}

/* private methods */

/*
 * Checks if the current item is selected, and notify a selection
 * change if the selection has changed
 */
static void
check_selected (UnityLauncherIconAccessible *self)
{
  LauncherIcon *icon = NULL;
  LauncherIcon *selected_icon = NULL;
  Launcher *launcher = NULL;
  nux::Object *nux_object = NULL;
  gboolean found = FALSE;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (self));

  if (nux_object == NULL) /* state is defunct */
    return;

  icon = dynamic_cast<LauncherIcon *>(nux_object);
  launcher = icon->GetLauncher ();

  if ((launcher == NULL) || (self->priv->parent_focused == FALSE))
    return;

  selected_icon = launcher->GetSelectedMenuIcon ();

  if (icon == selected_icon)
    found = TRUE;

  if (found != self->priv->selected)
    {
      gboolean return_val = FALSE;

      self->priv->selected = found;
      atk_object_notify_state_change (ATK_OBJECT (self),
                                      ATK_STATE_SELECTED,
                                      found);
      atk_object_notify_state_change (ATK_OBJECT (self),
                                      ATK_STATE_ACTIVE,
                                      found);

      g_signal_emit_by_name (self, "focus_event", found, &return_val);
      atk_focus_tracker_notify (ATK_OBJECT (self));

      g_debug ("[LAUNCHER-ICON]: selected state changed (%p:%i:%s)",
               self, found, atk_object_get_name (ATK_OBJECT (self)));
    }
}

static void
on_parent_selection_change_cb (AtkSelection *selection,
                               gpointer data)
{
  g_return_if_fail (UNITY_IS_LAUNCHER_ACCESSIBLE (selection));
  g_return_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (data));

  check_selected (UNITY_LAUNCHER_ICON_ACCESSIBLE (data));
}


static void
on_parent_focus_event_cb (AtkObject *object,
                          gboolean in,
                          gpointer data)
{
  UnityLauncherIconAccessible *self = NULL;

  g_return_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (data));

  self = UNITY_LAUNCHER_ICON_ACCESSIBLE (data);
  self->priv->parent_focused = in;

  /* we check the selection stuff again, to report the focus change
     now */
  check_selected (self);
}

/* AtkComponent.h */

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

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
unity_launcher_icon_accessible_add_focus_handler (AtkComponent *component,
                                                  AtkFocusHandler handler)
{
  GSignalMatchType match_type;
  gulong ret;
  guint signal_id;

  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (component), 0);

  match_type = (GSignalMatchType) (G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC);
  signal_id = g_signal_lookup ("focus-event", ATK_TYPE_OBJECT);

  ret = g_signal_handler_find (component, match_type, signal_id, 0, NULL,
                               (gpointer) handler, NULL);
  if (!ret)
    {
      return g_signal_connect_closure_by_id (component,
                                             signal_id, 0,
                                             g_cclosure_new (G_CALLBACK (handler), NULL,
                                                             (GClosureNotify) NULL),
                                             FALSE);
    }
  else
    return 0;
}

static void
unity_launcher_icon_accessible_remove_focus_handler (AtkComponent *component,
                                                     guint handler_id)
{
  g_return_if_fail (NUX_IS_VIEW_ACCESSIBLE (component));

  g_signal_handler_disconnect (component, handler_id);
}

static void
unity_launcher_icon_accessible_focus_handler (AtkObject *accessible,
                                              gboolean focus_in)
{
  g_return_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (accessible));

  g_debug ("[a11y] launcher_icon_focus_handler (%p:%s:%i)",
           accessible, atk_object_get_name (accessible), focus_in);

  atk_object_notify_state_change (accessible, ATK_STATE_FOCUSED, focus_in);
}

/* Public */
static gint
unity_launcher_icon_accessible_get_index_in_parent (AtkObject *obj)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (obj), -1);

  return UNITY_LAUNCHER_ICON_ACCESSIBLE (obj)->priv->index_in_parent;
}

void
unity_launcher_icon_accessible_set_index (UnityLauncherIconAccessible *self,
                                          gint index)
{
  g_return_if_fail (UNITY_IS_LAUNCHER_ICON_ACCESSIBLE (self));

  self->priv->index_in_parent = index;
}
