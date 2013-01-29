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
 * Right now it is used to:
 *  * Expose the child of BaseWindow (the layout)
 *  * Window event notification (activate, deactivate, and so on)
 *
 * BTW: we consider that one window is active, if it directly has
 * keyboard focus, or if one of its children has keyboard focus (ie:
 * the Launcher via GrabKeyboardFocus)
 *
 * HasKeyboardFocus is not reliable to check that:
 *  see bug https://bugs.launchpad.net/nux/+bug/745049
 *
 * So we need to update the state of the objects using the information
 * from the signals OnStartKeyboardReceiver and OnStopKeyboardReceiver
 *
 * #NuxBaseWindowAccessible implements the required ATK interfaces of
 * nux::BaseWindow, exposing as a child the BaseWindow layout
 *
 */

#include "nux-base-window-accessible.h"

#include <Nux/Area.h>
#include <Nux/Layout.h>

/* GObject */
static void nux_base_window_accessible_class_init(NuxBaseWindowAccessibleClass* klass);
static void nux_base_window_accessible_init(NuxBaseWindowAccessible* base_window_accessible);

/* AtkObject.h */
static void       nux_base_window_accessible_initialize(AtkObject* accessible,
                                                        gpointer   data);
static AtkObject* nux_base_window_accessible_get_parent(AtkObject* obj);
static AtkStateSet* nux_base_window_accessible_ref_state_set(AtkObject* obj);

/* AtkWindow.h */
static void         atk_window_interface_init(AtkWindowIface* iface);


G_DEFINE_TYPE_WITH_CODE(NuxBaseWindowAccessible, nux_base_window_accessible,
                        NUX_TYPE_VIEW_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_WINDOW,
                                              atk_window_interface_init))

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

  atk_object_set_role(accessible, ATK_ROLE_WINDOW);
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

  /* HasKeyboardFocus is not reliable here:
     see bug https://bugs.launchpad.net/nux/+bug/745049 */
  if (self->priv->active)
  {
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
  }

  return state_set;
}

/* AtkWindow */
static void
atk_window_interface_init(AtkWindowIface* iface)
{
  /* AtkWindow just defines signals at this moment */
}

/* public */
/*
 * Checks if we are the active window.
 */
void
nux_base_window_accessible_check_active(NuxBaseWindowAccessible* self,
                                        nux::BaseWindow* active_window)
{
  gboolean is_active;
  nux::Object* nux_object = NULL;
  nux::BaseWindow* bwindow = NULL;

  g_return_if_fail(NUX_IS_BASE_WINDOW_ACCESSIBLE(self));

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  bwindow = dynamic_cast<nux::BaseWindow*>(nux_object);
  if (bwindow == NULL) /* defunct */
    return;

  is_active = (bwindow == active_window);

  if (self->priv->active != is_active)
  {
    const gchar* signal_name;
    self->priv->active = is_active;

    if (is_active)
      signal_name = "activate";
    else
      signal_name = "deactivate";

    atk_object_notify_state_change(ATK_OBJECT(self),
                                   ATK_STATE_ACTIVE, is_active);
    g_signal_emit_by_name(self, signal_name, 0);
  }
}
