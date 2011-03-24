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
#include "unitya11y.h"

#include "Nux/Layout.h"
#include "Nux/Area.h"

/* GObject */
static void nux_view_accessible_class_init (NuxViewAccessibleClass *klass);
static void nux_view_accessible_init       (NuxViewAccessible *view_accessible);

/* AtkObject.h */
static void       nux_view_accessible_initialize     (AtkObject *accessible,
                                                      gpointer   data);

static AtkStateSet* nux_view_accessible_ref_state_set       (AtkObject *obj);
static gint         nux_view_accessible_get_n_children      (AtkObject *obj);
static AtkObject*   nux_view_accessible_ref_child           (AtkObject *obj,
                                                             gint i);

/* AtkComponent.h */
static void     atk_component_interface_init             (AtkComponentIface *iface);
static gboolean nux_view_accessible_grab_focus           (AtkComponent *component);
static guint    nux_view_accessible_add_focus_handler    (AtkComponent *component,
                                                          AtkFocusHandler handler);
static void     nux_view_accessible_remove_focus_handler (AtkComponent *component,
                                                          guint handler_id);

/* private methods */
static void nux_view_accessible_focus_handler (AtkObject *accessible,
                                               gboolean focus_in);
static void on_layout_changed_cb              (nux::View *view,
                                               nux::Layout *layout,
                                               AtkObject *accessible,
                                               gboolean is_add);
static void on_focus_changed_cb               (nux::Area *area,
                                               AtkObject *accessible);

G_DEFINE_TYPE_WITH_CODE (NuxViewAccessible,
                         nux_view_accessible,
                         NUX_TYPE_AREA_ACCESSIBLE,
                         G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                atk_component_interface_init))

#define NUX_VIEW_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_VIEW_ACCESSIBLE,        \
                                NuxViewAccessiblePrivate))

struct _NuxViewAccessiblePrivate
{
  /* Cached values (used to avoid extra notifications) */
  gboolean focused;
};


static void
nux_view_accessible_class_init (NuxViewAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_class = ATK_OBJECT_CLASS (klass);

  /* AtkObject */
  atk_class->initialize = nux_view_accessible_initialize;
  atk_class->ref_state_set = nux_view_accessible_ref_state_set;
  atk_class->ref_child = nux_view_accessible_ref_child;
  atk_class->get_n_children = nux_view_accessible_get_n_children;

  g_type_class_add_private (gobject_class, sizeof (NuxViewAccessiblePrivate));
}

static void
nux_view_accessible_init (NuxViewAccessible *view_accessible)
{
  NuxViewAccessiblePrivate *priv =
    NUX_VIEW_ACCESSIBLE_GET_PRIVATE (view_accessible);

  view_accessible->priv = priv;
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

  /* focus support based on Focusable, used on the Dash */
  view->FocusChanged.connect (sigc::bind (sigc::ptr_fun (on_focus_changed_cb), accessible));

  view->LayoutAdded.connect (sigc::bind (sigc::ptr_fun (on_layout_changed_cb),
                                         accessible, TRUE));
  view->LayoutRemoved.connect (sigc::bind (sigc::ptr_fun (on_layout_changed_cb),
                                           accessible, FALSE));

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

static gint
nux_view_accessible_get_n_children (AtkObject *obj)
{
  nux::Object *nux_object = NULL;
  nux::View *view = NULL;
  nux::Layout *layout = NULL;

  g_return_val_if_fail (NUX_IS_VIEW_ACCESSIBLE (obj), 0);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;

  view = dynamic_cast<nux::View *>(nux_object);

  layout = view->GetLayout ();

  if (layout == NULL)
    return 0;
  else
    return 1;
}

static AtkObject *
nux_view_accessible_ref_child (AtkObject *obj,
                               gint i)
{
  nux::Object *nux_object = NULL;
  nux::View *view = NULL;
  nux::Layout *layout = NULL;
  AtkObject *layout_accessible = NULL;
  gint num = 0;

  g_return_val_if_fail (NUX_IS_VIEW_ACCESSIBLE (obj), 0);

  num = atk_object_get_n_accessible_children (obj);
  g_return_val_if_fail ((i < num)&&(i >= 0), NULL);

  nux_object = nux_object_accessible_get_object (NUX_OBJECT_ACCESSIBLE (obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;

  view = dynamic_cast<nux::View *>(nux_object);

  layout = view->GetLayout ();

  layout_accessible = unity_a11y_get_accessible (layout);

  if (layout_accessible != NULL)
    g_object_ref (layout_accessible);

  return layout_accessible;
}

static void
on_layout_changed_cb (nux::View *view,
                      nux::Layout *layout,
                      AtkObject *accessible,
                      gboolean is_add)
{
  const gchar *signal_name = NULL;
  AtkObject *atk_child = NULL;

  g_return_if_fail (NUX_IS_VIEW_ACCESSIBLE (accessible));

  atk_child = unity_a11y_get_accessible (layout);

  g_debug ("[a11y][view] Layout change (%p:%s)(%p:%s)(%i)",
           accessible, atk_object_get_name (accessible),
           atk_child, atk_object_get_name (atk_child),
           is_add);

  if (is_add)
    signal_name = "children-changed::add";
  else
    signal_name = "children-changed::remove";

  /* index is always 0 as there is always just one layout */
  g_signal_emit_by_name (accessible, signal_name, 0, atk_child, NULL);
}

static void
on_focus_changed_cb (nux::Area *area,
                     AtkObject *accessible)
{
  gboolean focus_in = FALSE;
  NuxViewAccessible *self = NULL;

  g_return_if_fail (NUX_IS_VIEW_ACCESSIBLE (accessible));

  self = NUX_VIEW_ACCESSIBLE (accessible);

  if (area->GetFocused ())
    focus_in = TRUE;

  g_debug ("[a11y][view] on_focus_change_cb: (%p:%s:%i)",
           accessible, atk_object_get_name (accessible), focus_in);

  if (self->priv->focused != focus_in)
    {
      self->priv->focused = focus_in;

      g_signal_emit_by_name (accessible, "focus_event", focus_in);
      atk_focus_tracker_notify (accessible);
    }
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

  g_debug ("[a11y][view] focus_handler (%p:%s:%i)",
           accessible, atk_object_get_name (accessible), focus_in);

  atk_object_notify_state_change (accessible, ATK_STATE_FOCUSED, focus_in);
}
