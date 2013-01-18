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
 * SECTION:nux-area-accessible
 * @Title: NuxAreaAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::Area
 * @see_also: nux::Area
 *
 * #NuxAreaAccessible implements the required ATK interfaces of
 * nux::Area, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 * In this object the main support for the focused object is also
 * implemented. This is complex due to several reasons:
 *
 *  * We need to ensure the proper order when the objects get the focus
 *
 * * It doesn't make sense to give the focus to an object that is
 *    inside an inactive window too, so it is also convenient to emit
 *    the window:active event before the focus change.
 *
 * => this is the reason why there is a system to delay the focus
 *    notification, until the top level window becomes active, implemented
 *
 * * But the main complexity comes from the fact that not all the
 *   objects of Unity are implementing key nav the same way.
 *
 *     * The Launcher uses InputArea methods like
 *       SetKeyboardFocus, OnStartKeyboardReceiver, etc exclusively.
 *       This is the key focus at a low level abstraction
 *
 *     * Dash objects use the events from Focusable. But in the same
 *       way, they require the low level key focus (OnStartFocus) and
 *       so on
 *
 *
 */

#include "nux-area-accessible.h"

#include "unitya11y.h"

/* GObject */
static void nux_area_accessible_class_init(NuxAreaAccessibleClass* klass);
static void nux_area_accessible_init(NuxAreaAccessible* area_accessible);

/* AtkObject.h */
static void         nux_area_accessible_initialize(AtkObject* accessible,
                                                   gpointer   data);
static AtkObject*   nux_area_accessible_get_parent(AtkObject* obj);
static AtkStateSet* nux_area_accessible_ref_state_set(AtkObject* obj);

/* AtkComponent.h */
static void         atk_component_interface_init(AtkComponentIface* iface);
static void         nux_area_accessible_get_extents(AtkComponent* component,
                                                    gint*         x,
                                                    gint*         y,
                                                    gint*         width,
                                                    gint*         height,
                                                    AtkCoordType  coord_type);
static gboolean     nux_area_accessible_grab_focus(AtkComponent* component);
static guint        nux_area_accessible_add_focus_handler(AtkComponent* component,
                                                          AtkFocusHandler handler);
static void         nux_area_accessible_remove_focus_handler(AtkComponent* component,
                                                             guint handler_id);
static void         nux_area_accessible_focus_handler(AtkObject* accessible,
                                                      gboolean focus_in);
/* private */
static void          on_focus_changed_cb(nux::Area* area,
                                         bool has_focus,
                                         nux::KeyNavDirection direction,
                                         AtkObject* accessible);
static void          on_parent_window_activate_cb(AtkObject* parent_window,
                                                  NuxAreaAccessible* self);
static AtkObject*    search_for_parent_window(AtkObject* object);
static gboolean      nux_area_accessible_real_check_pending_notification(NuxAreaAccessible* self);
static void          check_parent_window_connected(NuxAreaAccessible* self);
static void          check_focus(NuxAreaAccessible* self);

G_DEFINE_TYPE_WITH_CODE(NuxAreaAccessible,
                        nux_area_accessible,
                        NUX_TYPE_OBJECT_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_COMPONENT,
                                              atk_component_interface_init))

#define NUX_AREA_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_AREA_ACCESSIBLE,        \
                                NuxAreaAccessiblePrivate))

struct _NuxAreaAccessiblePrivate
{
  /* focused as Focusable events */
  gboolean focused;

  /* if there is any pending notification */
  gboolean pending_notification;

  /* Top level parent window, it is not required to be the direct
     parent */
  AtkObject* parent_window;
};


static void
nux_area_accessible_class_init(NuxAreaAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);
  NuxAreaAccessibleClass* area_class = NUX_AREA_ACCESSIBLE_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = nux_area_accessible_initialize;
  atk_class->get_parent = nux_area_accessible_get_parent;
  atk_class->ref_state_set = nux_area_accessible_ref_state_set;

  /* NuxAreaAccessible */
  area_class->check_pending_notification = nux_area_accessible_real_check_pending_notification;

  g_type_class_add_private(gobject_class, sizeof(NuxAreaAccessiblePrivate));
}

static void
nux_area_accessible_init(NuxAreaAccessible* area_accessible)
{
  NuxAreaAccessiblePrivate* priv =
    NUX_AREA_ACCESSIBLE_GET_PRIVATE(area_accessible);

  area_accessible->priv = priv;
}

AtkObject*
nux_area_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<nux::Area*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(NUX_TYPE_AREA_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_area_accessible_initialize(AtkObject* accessible,
                               gpointer data)
{
  nux::Object* nux_object = NULL;
  nux::Area* area = NULL;

  ATK_OBJECT_CLASS(nux_area_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  area = dynamic_cast<nux::Area*>(nux_object);

  /* focus support based on Focusable, used on the Dash */
  area->key_nav_focus_change.connect(sigc::bind(sigc::ptr_fun(on_focus_changed_cb), accessible));

  atk_component_add_focus_handler(ATK_COMPONENT(accessible),
                                  nux_area_accessible_focus_handler);

  /* NOTE: we can't search for the parent window on initialization as a
     general rule, or we could enter an infinite loop. At area this
     is done on the focus event. On the Switcher this is done on their
     initialization itself */
}

static AtkObject*
nux_area_accessible_get_parent(AtkObject* obj)
{
  nux::Object* nux_object = NULL;
  nux::Area* area = NULL;
  nux::Area* parent = NULL;

  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(obj), NULL);

  if (obj->accessible_parent)
    return obj->accessible_parent;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return NULL;

  area = dynamic_cast<nux::Area*>(nux_object);

  parent = area->GetParentObject();

  return unity_a11y_get_accessible(parent);
}

/*
 * Checks if the parent actor, and his parent, etc are all visible
 * Used to check the showing state
 */
static gboolean
_check_all_parents_visible(nux::Area* area)
{
  nux::Area* iter_parent = NULL;
  gboolean result = TRUE;

  for (iter_parent = area; iter_parent;
       iter_parent = iter_parent->GetParentObject())
  {
    if (!iter_parent->IsVisible())
    {
      result = FALSE;
      break;
    }
  }

  return result;
}

static AtkStateSet*
nux_area_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;
  nux::Area* area = NULL;

  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(nux_area_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  area = dynamic_cast<nux::Area*>(nux_object);

  if (area->IsSensitive())
  {
    atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);
    atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  }

  if (area->IsVisible())
  {
    atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);

    if (_check_all_parents_visible(area))
      atk_state_set_add_state(state_set, ATK_STATE_SHOWING);
  }

  // FIXME CanFocus is no longer part of Nux API
//  if (area->CanFocus())
//    atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);

  if (area->HasKeyFocus())
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);

  return state_set;
}

/* AtkComponent implementation */
static void
atk_component_interface_init(AtkComponentIface* iface)
{
  g_return_if_fail(iface != NULL);

  /* placement */
  iface->get_extents    = nux_area_accessible_get_extents;

  /* focus management based on Focusable */
  iface->grab_focus           = nux_area_accessible_grab_focus;
  iface->add_focus_handler    = nux_area_accessible_add_focus_handler;
  iface->remove_focus_handler = nux_area_accessible_remove_focus_handler;
}

static void
nux_area_accessible_get_extents(AtkComponent* component,
                                gint*         x,
                                gint*         y,
                                gint*         width,
                                gint*         height,
                                AtkCoordType coord_type)
{
  gint top_level_x = 0;
  gint top_level_y = 0;
  nux::Object* nux_object = NULL;
  nux::Area* area = NULL;
  nux::Geometry geometry;

  g_return_if_fail(NUX_IS_AREA_ACCESSIBLE(component));

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(component));

  if (nux_object == NULL) /* defunct */
    return;

  area = dynamic_cast<nux::Area*>(nux_object);

  geometry = area->GetGeometry();

  *width = geometry.GetWidth();
  *height = geometry.GetWidth();
  *x = geometry.x;
  *y = geometry.y;

  /* In the ATK_XY_WINDOW case
   *
   * http://library.gnome.org/devel/atk/stable/AtkUtil.html#AtkCoordType
   */

  if (coord_type == ATK_XY_SCREEN)
  {
    /* For the moment Unity is a full-screen app, so ATK_XY_SCREEN
       and ATK_XY_WINDOW are the same */
    *x += top_level_x;
    *y += top_level_y;
  }

  return;
}

static gboolean
nux_area_accessible_grab_focus(AtkComponent* component)
{
  nux::Object* nux_object = NULL;
  //nux::Area* area = NULL;

  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(component), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(component));
  if (nux_object == NULL) /* defunct */
    return FALSE;

  //area = dynamic_cast<nux::Area*>(nux_object);

  /* FIXME: SetFocused doesn't return if the force was successful or
     not, we suppose that this is the case like in cally and gail */

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
nux_area_accessible_add_focus_handler(AtkComponent* component,
                                      AtkFocusHandler handler)
{
  GSignalMatchType match_type;
  gulong ret;
  guint signal_id;

  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(component), 0);

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
nux_area_accessible_remove_focus_handler(AtkComponent* component,
                                         guint handler_id)
{
  g_return_if_fail(NUX_IS_AREA_ACCESSIBLE(component));

  g_signal_handler_disconnect(component, handler_id);
}

static void
nux_area_accessible_focus_handler(AtkObject* accessible,
                                  gboolean focus_in)
{
  g_return_if_fail(NUX_IS_AREA_ACCESSIBLE(accessible));

  atk_object_notify_state_change(accessible, ATK_STATE_FOCUSED, focus_in);
}

/* private */
static void
check_parent_window_connected(NuxAreaAccessible* self)
{
  AtkObject* window = NULL;

  if (self->priv->parent_window != NULL)
    return;

  window = search_for_parent_window(ATK_OBJECT(self));

  if (window != NULL)
  {
    self->priv->parent_window = window;

    g_signal_connect(self->priv->parent_window,
                     "activate",
                     G_CALLBACK(on_parent_window_activate_cb),
                     self);
  }
}

/*
 * nux_area_accessible_parent_window_active
 * @self: The accessible to check the focus change
 *
 * Returns if the top level parent window contains
 * the state ATK_STATE_ACTIVE
 *
 * Returns: TRUE if the parent top level window contains
 * ATK_STATE_ACTIVE, FALSE otherwise
 */
gboolean
nux_area_accessible_parent_window_active(NuxAreaAccessible* self)
{
  AtkStateSet* state_set = NULL;

  check_parent_window_connected(self);

  state_set = atk_object_ref_state_set(ATK_OBJECT(self->priv->parent_window));

  gboolean active = atk_state_set_contains_state(state_set, ATK_STATE_ACTIVE);

  g_object_unref(state_set);

  return active;
}

static void
on_focus_changed_cb(nux::Area* area,
                    bool has_focus,
                    nux::KeyNavDirection direction,
                    AtkObject* accessible)
{
  check_focus(NUX_AREA_ACCESSIBLE(accessible));
}

/* Check to use GetTopLevelViewWindow */
static AtkObject*
search_for_parent_window(AtkObject* object)
{
  AtkObject* parent = NULL;

  for (parent = atk_object_get_parent(object);
       (parent != NULL) && (atk_object_get_role(parent) != ATK_ROLE_WINDOW);
       parent = atk_object_get_parent(parent));

  return parent;
}

static void
on_parent_window_activate_cb(AtkObject* parent_window,
                             NuxAreaAccessible* self)
{
  nux_area_accessible_check_pending_notification(self);
}


/*
 * nux_area_check_pending_notification:
 * @self: The accessible
 *
 * This method checks if there is any pending notification, and emits
 * it if it is possible
 *
 * Returns: TRUE if an atk notification was emitted, FALSE otherwise
 */
gboolean
nux_area_accessible_check_pending_notification(NuxAreaAccessible* self)
{
  NuxAreaAccessibleClass* klass = NULL;

  klass = NUX_AREA_ACCESSIBLE_GET_CLASS(self);
  if (klass->check_pending_notification)
    return klass->check_pending_notification(self);
  else
    return FALSE;
}

static gboolean
nux_area_accessible_real_check_pending_notification(NuxAreaAccessible* self)
{
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(self), FALSE);

  if (self->priv->pending_notification == FALSE)
    return FALSE;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (nux_object == NULL) /* defunct */
    return FALSE;

  g_signal_emit_by_name(self, "focus_event", self->priv->focused);
  atk_focus_tracker_notify(ATK_OBJECT(self));
  self->priv->pending_notification = FALSE;

  return TRUE;
}

static void
check_focus(NuxAreaAccessible* self)
{
  gboolean focus_in = FALSE;
  nux::Area* area = NULL;
  nux::Object* nux_object = NULL;

  g_return_if_fail(NUX_IS_AREA_ACCESSIBLE(self));

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (nux_object == NULL) /* defunct */
    return;

  area = dynamic_cast<nux::Area*>(nux_object);

  if (nux::GetWindowCompositor().GetKeyFocusArea() == area)
    focus_in = TRUE;

  if (self->priv->focused != focus_in)
  {
    self->priv->focused = focus_in;
    gboolean is_parent_window_active = nux_area_accessible_parent_window_active(self);

    /* we don't emit focus_in=TRUE events until the top level window
       is active */
    if ((focus_in) && (!is_parent_window_active))
    {
      self->priv->pending_notification = TRUE;
    }
    else
    {
      g_signal_emit_by_name(self, "focus_event", focus_in);
      atk_focus_tracker_notify(ATK_OBJECT(self));
      self->priv->pending_notification = FALSE;
    }
  }
}


/* public */
/*
 * nux_area_get_parent_window:
 * @self: The accessible
 *
 * Returns: the top level window that contains this object
 */
AtkObject*
nux_area_accessible_get_parent_window(NuxAreaAccessible* self)
{
  g_return_val_if_fail(NUX_IS_AREA_ACCESSIBLE(self), NULL);

  /* At least ensure that we made a search for it */
  check_parent_window_connected(self);

  return self->priv->parent_window;
}
