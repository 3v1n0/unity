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
#include "nux-base-window-accessible.h"

#include <Nux/Layout.h>
#include <Nux/Area.h>

/* GObject */
static void nux_view_accessible_class_init(NuxViewAccessibleClass* klass);
static void nux_view_accessible_init(NuxViewAccessible* view_accessible);

/* AtkObject.h */
static void         nux_view_accessible_initialize(AtkObject* accessible,
                                                   gpointer   data);
static AtkStateSet* nux_view_accessible_ref_state_set(AtkObject* obj);
static gint         nux_view_accessible_get_n_children(AtkObject* obj);
static AtkObject*   nux_view_accessible_ref_child(AtkObject* obj,
                                                  gint i);
static AtkStateSet* nux_view_accessible_ref_state_set(AtkObject* obj);
static gint         nux_view_accessible_get_n_children(AtkObject* obj);
static AtkObject*   nux_view_accessible_ref_child(AtkObject* obj,
                                                  gint i);
/* NuxAreaAccessible */
static gboolean      nux_view_accessible_check_pending_notification(NuxAreaAccessible* self);

/* private methods */
static void on_layout_changed_cb(nux::View* view,
                                 nux::Layout* layout,
                                 AtkObject* accessible,
                                 gboolean is_add);
static void on_change_keyboard_receiver_cb(AtkObject* accessible,
                                           gboolean focus_in);

G_DEFINE_TYPE(NuxViewAccessible,
              nux_view_accessible,
              NUX_TYPE_AREA_ACCESSIBLE)

#define NUX_VIEW_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NUX_TYPE_VIEW_ACCESSIBLE,        \
                                NuxViewAccessiblePrivate))

struct _NuxViewAccessiblePrivate
{
  /* focused using InputArea OnStartKeyboardReceiver and OnStop... signals */
  gboolean key_focused;

  /* if the state from key_focused was notified or not */
  gboolean pending_notification;
};


static void
nux_view_accessible_class_init(NuxViewAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);
  NuxAreaAccessibleClass* area_class = NUX_AREA_ACCESSIBLE_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = nux_view_accessible_initialize;
  atk_class->ref_state_set = nux_view_accessible_ref_state_set;
  atk_class->ref_child = nux_view_accessible_ref_child;
  atk_class->get_n_children = nux_view_accessible_get_n_children;

  /* NuxAreaAccessible */
  area_class->check_pending_notification = nux_view_accessible_check_pending_notification;

  g_type_class_add_private(gobject_class, sizeof(NuxViewAccessiblePrivate));
}

static void
nux_view_accessible_init(NuxViewAccessible* view_accessible)
{
  NuxViewAccessiblePrivate* priv =
    NUX_VIEW_ACCESSIBLE_GET_PRIVATE(view_accessible);

  view_accessible->priv = priv;
}

AtkObject*
nux_view_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<nux::View*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(NUX_TYPE_VIEW_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_view_accessible_initialize(AtkObject* accessible,
                               gpointer data)
{
  nux::Object* nux_object = NULL;
  nux::View* view = NULL;

  ATK_OBJECT_CLASS(nux_view_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_UNKNOWN;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  view = dynamic_cast<nux::View*>(nux_object);

  view->LayoutAdded.connect(sigc::bind(sigc::ptr_fun(on_layout_changed_cb),
                                       accessible, TRUE));
  view->LayoutRemoved.connect(sigc::bind(sigc::ptr_fun(on_layout_changed_cb),
                                         accessible, FALSE));

  /* Some extra focus things as Focusable is not used on Launcher and
     some BaseWindow */
  view->begin_key_focus.connect(sigc::bind(sigc::ptr_fun(on_change_keyboard_receiver_cb),
                                           accessible, TRUE));
  view->end_key_focus.connect(sigc::bind(sigc::ptr_fun(on_change_keyboard_receiver_cb),
                                         accessible, FALSE));
}

static AtkStateSet*
nux_view_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;
  NuxViewAccessible* self = NULL;

  g_return_val_if_fail(NUX_IS_VIEW_ACCESSIBLE(obj), NULL);
  self = NUX_VIEW_ACCESSIBLE(obj);

  state_set = ATK_OBJECT_CLASS(nux_view_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  /* HasKeyboardFocus is not reliable here:
     see bug https://bugs.launchpad.net/nux/+bug/745049 */
  if (self->priv->key_focused)
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);

  return state_set;
}

static gint
nux_view_accessible_get_n_children(AtkObject* obj)
{
  nux::Object* nux_object = NULL;
  nux::View* view = NULL;
  nux::Layout* layout = NULL;

  g_return_val_if_fail(NUX_IS_VIEW_ACCESSIBLE(obj), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;

  view = dynamic_cast<nux::View*>(nux_object);

  layout = view->GetLayout();

  if (layout == NULL)
    return 0;
  else
    return 1;
}

static AtkObject*
nux_view_accessible_ref_child(AtkObject* obj,
                              gint i)
{
  nux::Object* nux_object = NULL;
  nux::View* view = NULL;
  nux::Layout* layout = NULL;
  AtkObject* layout_accessible = NULL;
  gint num = 0;

  g_return_val_if_fail(NUX_IS_VIEW_ACCESSIBLE(obj), 0);

  num = atk_object_get_n_accessible_children(obj);
  g_return_val_if_fail((i < num) && (i >= 0), NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (nux_object == NULL) /* state is defunct */
    return 0;

  view = dynamic_cast<nux::View*>(nux_object);

  layout = view->GetLayout();

  layout_accessible = unity_a11y_get_accessible(layout);

  if (layout_accessible != NULL)
    g_object_ref(layout_accessible);

  return layout_accessible;
}

static void
on_layout_changed_cb(nux::View* view,
                     nux::Layout* layout,
                     AtkObject* accessible,
                     gboolean is_add)
{
  const gchar* signal_name = NULL;
  AtkObject* atk_child = NULL;

  g_return_if_fail(NUX_IS_VIEW_ACCESSIBLE(accessible));

  atk_child = unity_a11y_get_accessible(layout);

  if (is_add)
    signal_name = "children-changed::add";
  else
    signal_name = "children-changed::remove";

  /* index is always 0 as there is always just one layout */
  g_signal_emit_by_name(accessible, signal_name, 0, atk_child, NULL);
}

static void
on_change_keyboard_receiver_cb(AtkObject* accessible,
                               gboolean focus_in)
{
  NuxViewAccessible* self = NULL;

  g_return_if_fail(NUX_IS_VIEW_ACCESSIBLE(accessible));
  self = NUX_VIEW_ACCESSIBLE(accessible);

  if (self->priv->key_focused != focus_in)
  {
    self->priv->key_focused = focus_in;

    /* we always lead the focus notification to
       _check_pending_notification, in order to allow the proper
       window_activate -> focus_change order */
    self->priv->pending_notification = TRUE;
  }
}

static gboolean
nux_view_accessible_check_pending_notification(NuxAreaAccessible* area_accessible)
{
  NuxViewAccessible* self = NULL;
  nux::Object* nux_object = NULL;

  /* We also call the parent implementation, as we are not totally
     overriding check_pending_notification, just adding extra
     functionality*/
  NUX_AREA_ACCESSIBLE_CLASS(nux_view_accessible_parent_class)->check_pending_notification(area_accessible);

  g_return_val_if_fail(NUX_IS_VIEW_ACCESSIBLE(area_accessible), FALSE);
  self = NUX_VIEW_ACCESSIBLE(area_accessible);

  if (self->priv->pending_notification == FALSE)
    return FALSE;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (nux_object == NULL) /* defunct */
    return FALSE;

  g_signal_emit_by_name(self, "focus_event", self->priv->key_focused);
  atk_focus_tracker_notify(ATK_OBJECT(self));
  self->priv->pending_notification = FALSE;

  return TRUE;
}
