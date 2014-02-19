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
 * Authored by: Luke Yelavich <luke.yelavich@canonical.com>
 */

/**
 * SECTION:unity-session_button_accessible
 * @Title: UnitySessionButtonAccessible
 * @short_description: Implementation of the ATK interfaces for #unity::session::Button
 *
 * #UnitySessionButtonAccessible implements the required ATK interfaces of
 * unity::Button, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 */

#include "unity-session-button-accessible.h"
#include "SessionButton.h"

#include "unitya11y.h"

using namespace unity::session;

/* GObject */
static void unity_session_button_accessible_class_init(UnitySessionButtonAccessibleClass* klass);
static void unity_session_button_accessible_init(UnitySessionButtonAccessible* session_button_accessible);
static void unity_session_button_accessible_dispose(GObject* object);
static void unity_session_button_accessible_finalize(GObject* object);


/* AtkObject.h */
static void          unity_session_button_accessible_initialize(AtkObject* accessible,
                                                               gpointer   data);
static AtkStateSet*  unity_session_button_accessible_ref_state_set(AtkObject* obj);
static const gchar* unity_session_button_accessible_get_name(AtkObject* obj);


/* AtkAction */
static void         atk_action_interface_init(AtkActionIface *iface);
static gboolean     unity_session_button_accessible_do_action(AtkAction *action,
                                                              gint i);
static gint         unity_session_button_accessible_get_n_actions(AtkAction *action);
static const gchar* unity_session_button_accessible_get_name(AtkAction *action,
                                                            gint i);

/* private/utility methods*/
static void on_focus_change_cb(UnitySessionButtonAccessible* accessible);

G_DEFINE_TYPE_WITH_CODE(UnitySessionButtonAccessible,
                        unity_session_button_accessible,
                        NUX_TYPE_OBJECT_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_ACTION,
                                              atk_action_interface_init))

#define UNITY_SESSION_BUTTON_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE, \
                                UnitySessionButtonAccessiblePrivate))

struct _UnitySessionButtonAccessiblePrivate
{
  sigc::connection on_focus_change;
};

static void
unity_session_button_accessible_class_init(UnitySessionButtonAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_session_button_accessible_dispose;
  gobject_class->finalize = unity_session_button_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_session_button_accessible_initialize;
  atk_class->get_name = unity_session_button_accessible_get_name;
  atk_class->ref_state_set = unity_session_button_accessible_ref_state_set;

  g_type_class_add_private(gobject_class, sizeof(UnitySessionButtonAccessiblePrivate));
}

static void
unity_session_button_accessible_init(UnitySessionButtonAccessible* session_button_accessible)
{
  UnitySessionButtonAccessiblePrivate* priv =
    UNITY_SESSION_BUTTON_ACCESSIBLE_GET_PRIVATE(session_button_accessible);

  session_button_accessible->priv = priv;
}

static void
unity_session_button_accessible_dispose(GObject* object)
{
  G_OBJECT_CLASS(unity_session_button_accessible_parent_class)->dispose(object);
}

static void
unity_session_button_accessible_finalize(GObject* object)
{
  UnitySessionButtonAccessible* self = UNITY_SESSION_BUTTON_ACCESSIBLE(object);

  self->priv->on_focus_change.disconnect();

  G_OBJECT_CLASS(unity_session_button_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_session_button_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<Button*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_SESSION_BUTTON_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_session_button_accessible_initialize(AtkObject* accessible,
                                          gpointer data)
{
  UnitySessionButtonAccessible* self = NULL;
  nux::Object* nux_object = NULL;
  Button* button = NULL;

  ATK_OBJECT_CLASS(unity_session_button_accessible_parent_class)->initialize(accessible, data);
  self = UNITY_SESSION_BUTTON_ACCESSIBLE(accessible);

  accessible->role = ATK_ROLE_PUSH_BUTTON;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));

  if (nux_object == NULL) /* defunct */
    return;

  button = dynamic_cast<Button*>(nux_object);

  self->priv->on_focus_change = button->highlight_change.connect(sigc::bind(sigc::ptr_fun(on_focus_change_cb), self));
}

static const gchar*
unity_session_button_accessible_get_name(AtkObject* obj)
{
  const gchar* name;

  g_return_val_if_fail(UNITY_IS_SESSION_BUTTON_ACCESSIBLE(obj), NULL);

  name = ATK_OBJECT_CLASS(unity_session_button_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    Button* button = NULL;

    button = dynamic_cast<Button*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));

    if (button == NULL) /* State is defunct */
      name = NULL;
    else
      name = button->label().c_str();
  }

  return name;
}

static AtkStateSet*
unity_session_button_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;
  Button* button = NULL;

  g_return_val_if_fail(UNITY_IS_SESSION_BUTTON_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(unity_session_button_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  button = dynamic_cast<Button*>(nux_object);

  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
  atk_state_set_add_state(state_set, ATK_STATE_SHOWING);

  if (button->highlighted)
  {
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
    atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
  }

  return state_set;
}

/* private methods */
static void
on_focus_change_cb(UnitySessionButtonAccessible* accessible)
{
  nux::Object* nux_object = NULL;
  Button* button = NULL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));

  if (nux_object == NULL) /* defunct */
    return;

  button = dynamic_cast<Button*>(nux_object);

  atk_object_notify_state_change(ATK_OBJECT(accessible), ATK_STATE_FOCUSED, button->highlighted);
  atk_object_notify_state_change(ATK_OBJECT(accessible), ATK_STATE_SELECTED, button->highlighted);
  atk_object_notify_state_change(ATK_OBJECT(accessible), ATK_STATE_ACTIVE, button->highlighted);
}

/* AtkAction */
static void
atk_action_interface_init(AtkActionIface *iface)
{
  iface->do_action = unity_session_button_accessible_do_action;
  iface->get_n_actions = unity_session_button_accessible_get_n_actions;
  iface->get_name = unity_session_button_accessible_get_name;
}

static gboolean
unity_session_button_accessible_do_action(AtkAction *action,
                                         gint i)
{
  Button* button = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_SESSION_BUTTON_ACCESSIBLE(action), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(action));
  if (nux_object == NULL)
    return FALSE;

  button = dynamic_cast<Button*>(nux_object);

  button->activated.emit();

  return TRUE;
}

static gint
unity_session_button_accessible_get_n_actions(AtkAction *action)
{
  g_return_val_if_fail(UNITY_IS_SESSION_BUTTON_ACCESSIBLE(action), 0);

  return 1;
}

static const gchar*
unity_session_button_accessible_get_name(AtkAction *action,
                                        gint i)
{
  g_return_val_if_fail(UNITY_IS_SESSION_BUTTON_ACCESSIBLE(action), NULL);
  g_return_val_if_fail(i == 0, NULL);

  return "activate";
}
