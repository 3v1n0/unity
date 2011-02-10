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
 * SECTION:nux-view-accessible
 * @Title: NuxViewAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::View
 * @see_also: nux::View
 *
 * #NuxViewAccessible implements the required ATK interfaces of
 * nux::View
 *
 */

#include "nux-view-accessible.h"

/* GObject */
static void nux_view_accessible_class_init (NuxViewAccessibleClass *klass);
static void nux_view_accessible_init       (NuxViewAccessible *view_accessible);

/* AtkObject.h */
static void       nux_view_accessible_initialize     (AtkObject *accessible,
                                                      gpointer   data);

static AtkStateSet* nux_view_accessible_ref_state_set       (AtkObject *obj);

/* AtkComponent.h */
static void     atk_component_interface_init             (AtkComponentIface *iface);
static gboolean nux_view_accessible_grab_focus           (AtkComponent *component);
static guint    nux_view_accessible_add_focus_handler    (AtkComponent *component,
                                                          AtkFocusHandler handler);
static void     nux_view_accessible_remove_focus_handler (AtkComponent *component,
                                                          guint handler_id);

/* private methods */
static void on_start_focus_cb                 (AtkObject *accessible);
static void on_end_focus_cb                   (AtkObject *accessible);
static void nux_view_accessible_focus_handler (AtkObject *accessible,
                                               gboolean focus_in);

#define NUX_VIEW_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_VIEW_ACCESSIBLE, NuxViewAccessiblePrivate))

G_DEFINE_TYPE_WITH_CODE (NuxViewAccessible,
                         nux_view_accessible,
                         NUX_TYPE_AREA_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                atk_component_interface_init))

struct _NuxViewAccessiblePrivate
{
};

static void
nux_view_accessible_class_init (NuxViewAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_view_accessible_initialize;
  atk_class->ref_state_set = nux_view_accessible_ref_state_set;

  g_type_class_add_private (gobject_class, sizeof (NuxViewAccessiblePrivate));
}

static void
nux_view_accessible_init (NuxViewAccessible *view_accessible)
{
  view_accessible->priv = NUX_VIEW_ACCESSIBLE_GET_PRIVATE (view_accessible);
}

AtkObject*
nux_view_accessible_new (nux::Object *object)
{
  AtkObject *accessible = NULL;

  g_return_val_if_fail (dynamic_cast<nux::View *>(object), NULL);

  accessible = ATK_OBJECT (g_object_new (NUX_TYPE_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize (accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_view_accessible_initialize (AtkObject *accessible,
                                gpointer data)
{
  nux::Object *nux_object = NULL;
  nux::View *view = NULL;

  ATK_OBJECT_CLASS (nux_view_accessible_parent_class)->initialize (accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (accessible));
  view = dynamic_cast<nux::View *>(nux_object);

  view->OnStartFocus.connect (sigc::bind (sigc::ptr_fun (on_start_focus_cb), accessible));
  view->OnEndFocus.connect (sigc::bind (sigc::ptr_fun (on_end_focus_cb), accessible));

  atk_component_add_focus_handler (ATK_COMPONENT (accessible),
                                   nux_view_accessible_focus_handler);
}

static AtkStateSet*
nux_view_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  nux::Object *nux_object = NULL;
  nux::View *view = NULL;

  g_return_val_if_fail (NUX_IS_VIEW_ACCESSIBLE (obj), NULL);

  state_set = ATK_OBJECT_CLASS (nux_view_accessible_parent_class)->ref_state_set (obj);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));

  if (nux_object == NULL) /* actor is defunct */
    return state_set;

  view = dynamic_cast<nux::View *>(nux_object);

  /* FIXME: Waiting for the full keyboard navigation support, for the
     moment any nux object is focusable*/
  atk_state_set_add_state (state_set, ATK_STATE_FOCUSABLE);

  if (view->HasKeyboardFocus ())
    atk_state_set_add_state (state_set, ATK_STATE_FOCUSED);

  return state_set;
}

static void
on_start_focus_cb (AtkObject *accessible)
{
  g_signal_emit_by_name (accessible, "focus_event", TRUE);
  atk_focus_tracker_notify (accessible);
}

static void
on_end_focus_cb (AtkObject *accessible)
{
  g_signal_emit_by_name (accessible, "focus_event", FALSE);
  atk_focus_tracker_notify (accessible);
}

/* AtkComponent.h */
static void
atk_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  /* focus management */
  iface->grab_focus           = nux_view_accessible_grab_focus;
  iface->add_focus_handler    = nux_view_accessible_add_focus_handler;
  iface->remove_focus_handler = nux_view_accessible_remove_focus_handler;
}

static gboolean
nux_view_accessible_grab_focus (AtkComponent *component)
{
  nux::Object *nux_object = NULL;
  nux::View *view = NULL;

  g_return_val_if_fail (NUX_IS_VIEW_ACCESSIBLE (component), FALSE);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (component));
  if (nux_object == NULL) /* actor is defunct */
    return FALSE;

  view = dynamic_cast<nux::View *>(nux_object);

  view->ForceStartFocus (0, 0);

  /* FIXME: ForceStartFocus doesn't return if the force was succesful
     or not, we suppose that this is the case like in cally and gail */
  return TRUE;
}

/*
 * comment C&P from cally-actor:
 *
 * "These methods are basically taken from gail, as I don't see any
 * reason to modify it. It makes me wonder why it is really required
 * to be implemented in the toolkit"
 */

static guint
nux_view_accessible_add_focus_handler (AtkComponent *component,
                                       AtkFocusHandler handler)
{
  GSignalMatchType match_type;
  gulong ret;
  guint signal_id;

  g_return_val_if_fail (NUX_IS_VIEW_ACCESSIBLE (component), 0);

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
nux_view_accessible_remove_focus_handler (AtkComponent *component,
                                          guint handler_id)
{
  g_return_if_fail (NUX_IS_VIEW_ACCESSIBLE (component));

  g_signal_handler_disconnect (component, handler_id);
}

static void
nux_view_accessible_focus_handler (AtkObject *accessible,
                                   gboolean focus_in)
{
  g_return_if_fail (NUX_IS_VIEW_ACCESSIBLE (accessible));

  atk_object_notify_state_change (accessible, ATK_STATE_FOCUSED, focus_in);
}
