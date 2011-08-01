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
 * SECTION:nux-base_window-accessible
 * @Title: NuxBaseWindowAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::BaseWindow
 * @see_also: nux::BaseWindow
 *
 * Right now it is only here to expose the child of BaseWindow (the layout)
 *
 * #NuxBaseWindowAccessible implements the required ATK interfaces of
 * nux::BaseWindow, exposing as a child the BaseWindow layout
 *
 */

#include "nux-base-window-accessible.h"

#include "Nux/Area.h"
#include "Nux/Layout.h"

enum
{
  ACTIVATE,
  DEACTIVATE,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

/* GObject */
static void nux_base_window_accessible_class_init(NuxBaseWindowAccessibleClass* klass);
static void nux_base_window_accessible_init(NuxBaseWindowAccessible* base_window_accessible);

/* AtkObject.h */
static void       nux_base_window_accessible_initialize(AtkObject* accessible,
                                                        gpointer   data);
static AtkObject* nux_base_window_accessible_get_parent(AtkObject* obj);
static AtkStateSet* nux_base_window_accessible_ref_state_set(AtkObject* obj);

/* private */
static void       on_focus_event_cb(AtkObject* object,
                                    gboolean in,
                                    gpointer data);

G_DEFINE_TYPE(NuxBaseWindowAccessible, nux_base_window_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

struct _NuxBaseWindowAccessiblePrivate
{
  /* Cached values (used to avoid extra notifications) */
  gboolean active;
};

#define NUX_BASE_WINDOW_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_BASE_WINDOW_ACCESSIBLE, \
                                NuxBaseWindowAccessiblePrivate))

static void
nux_base_window_accessible_class_init(NuxBaseWindowAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = nux_base_window_accessible_initialize;
  atk_class->get_parent = nux_base_window_accessible_get_parent;
  atk_class->ref_state_set = nux_base_window_accessible_ref_state_set;

  g_type_class_add_private(gobject_class, sizeof(NuxBaseWindowAccessiblePrivate));

  /**
   * BaseWindow::activate:
   * @nux_object: the object which received the signal
   *
   * The ::activate signal is emitted when the window receives the key
   * focus from the underlying window system.
   *
   * Toolkit implementation note: it is used when anyone adds a global
   * event listener to "window:activate"
   */
  signals [ACTIVATE] =
    g_signal_new("activate",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0, /* default signal handler */
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

  /**
   * BaseWindow::deactivate:
   * @nux_object: the object which received the signal
   *
   * The ::deactivate signal is emitted when the window loses key
   * focus from the underlying window system.
   *
   * Toolkit implementation note: it is used when anyone adds a global
   * event listener to "window:deactivate"
   */
  signals [DEACTIVATE] =
    g_signal_new("deactivate",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 0, /* default signal handler */
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

}

static void
nux_base_window_accessible_init(NuxBaseWindowAccessible* base_window_accessible)
{
  NuxBaseWindowAccessiblePrivate* priv =
    NUX_BASE_WINDOW_ACCESSIBLE_GET_PRIVATE(base_window_accessible);

  base_window_accessible->priv = priv;
}

AtkObject*
nux_base_window_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<nux::BaseWindow*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(NUX_TYPE_BASE_WINDOW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_base_window_accessible_initialize(AtkObject* accessible,
                                      gpointer data)
{
  ATK_OBJECT_CLASS(nux_base_window_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_WINDOW;

  g_signal_connect(accessible, "focus-event",
                   G_CALLBACK(on_focus_event_cb), NULL);
}

static AtkObject*
nux_base_window_accessible_get_parent(AtkObject* obj)
{
  return atk_get_root();
}

static AtkStateSet*
nux_base_window_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  NuxBaseWindowAccessible* self = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(NUX_IS_BASE_WINDOW_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(nux_base_window_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (nux_object == NULL) /* defunct */
    return state_set;

  self = NUX_BASE_WINDOW_ACCESSIBLE(obj);

  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);

  if (self->priv->active)
  {
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
  }

  return state_set;
}

/* private */
static void
on_focus_event_cb(AtkObject* object,
                  gboolean focus_in,
                  gpointer data)
{
  NuxBaseWindowAccessible* self = NULL;

  /* On the base window, we suppose that the window is active if it
     has the focus*/
  self = NUX_BASE_WINDOW_ACCESSIBLE(object);

  if (self->priv->active != focus_in)
  {
    gint signal_id;

    self->priv->active = focus_in;

    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_ACTIVE, focus_in);

    if (focus_in)
      signal_id = ACTIVATE;
    else
      signal_id = DEACTIVATE;

    g_debug("[a11y][bwindow] on_focus_event activate events (%p:%s:%i)",
            object, atk_object_get_name(object), focus_in);

    g_signal_emit(self, signals [signal_id], 0);
  }
}
