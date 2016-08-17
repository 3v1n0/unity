/*
 * Copyright (C) 2015 Canonical Ltd
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
 * SECTION:unity-filter-basic-button_accessible
 * @Title: UnityFilterBasicButtonAccessible
 * @short_description: Implementation of the ATK interfaces for #unity::dash::FilterBasicButton
 *
 * #UnityFilterBasicButtonAccessible implements the required ATK interfaces of
 * unity::dash::FilterBasicButton, exposing the common elements on each basic individual
 * element (position, extents, etc)
 *
 */

#include <NuxCore/Logger.h>
#include "unity-filter-basic-button-accessible.h"
#include "FilterBasicButton.h"

#include "unitya11y.h"

DECLARE_LOGGER(logger, "unity.a11y.UnityFilterBasicButtonAccessible");

using namespace unity::dash;

/* GObject */
static void unity_filter_basic_button_accessible_class_init(UnityFilterBasicButtonAccessibleClass* klass);
static void unity_filter_basic_button_accessible_init(UnityFilterBasicButtonAccessible* session_button_accessible);
static void unity_filter_basic_button_accessible_dispose(GObject* object);
static void unity_filter_basic_button_accessible_finalize(GObject* object);


/* AtkObject.h */
static void          unity_filter_basic_button_accessible_initialize(AtkObject* accessible,
                                                               gpointer   data);
static AtkStateSet*  unity_filter_basic_button_accessible_ref_state_set(AtkObject* obj);
static const gchar* unity_filter_basic_button_accessible_get_name(AtkObject* obj);


/* AtkAction */
static void         atk_action_interface_init(AtkActionIface *iface);
static gboolean     unity_filter_basic_button_accessible_do_action(AtkAction *action,
                                                              gint i);
static gint         unity_filter_basic_button_accessible_get_n_actions(AtkAction *action);
static const gchar* unity_filter_basic_button_accessible_get_action_name(AtkAction *action,
                                                            gint i);
static void on_layout_changed_cb(nux::View* view,
                                 nux::Layout* layout,
                                 AtkObject* accessible,
                                 gboolean is_add);
static void on_focus_changed_cb(nux::Area* area,
                                bool has_focus,
                                nux::KeyNavDirection direction,
                                AtkObject* accessible);

G_DEFINE_TYPE_WITH_CODE(UnityFilterBasicButtonAccessible,
                        unity_filter_basic_button_accessible,
                        NUX_TYPE_VIEW_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_ACTION,
                                              atk_action_interface_init))

static void
unity_filter_basic_button_accessible_class_init(UnityFilterBasicButtonAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_filter_basic_button_accessible_dispose;
  gobject_class->finalize = unity_filter_basic_button_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_filter_basic_button_accessible_initialize;
  atk_class->get_name = unity_filter_basic_button_accessible_get_name;
  atk_class->ref_state_set = unity_filter_basic_button_accessible_ref_state_set;
}

static void
unity_filter_basic_button_accessible_init(UnityFilterBasicButtonAccessible* session_button_accessible)
{
}

static void
unity_filter_basic_button_accessible_dispose(GObject* object)
{
  G_OBJECT_CLASS(unity_filter_basic_button_accessible_parent_class)->dispose(object);
}

static void
unity_filter_basic_button_accessible_finalize(GObject* object)
{
  G_OBJECT_CLASS(unity_filter_basic_button_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_filter_basic_button_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<FilterBasicButton*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_FILTER_BASIC_BUTTON_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
unity_filter_basic_button_accessible_initialize(AtkObject* accessible,
                                          gpointer data)
{
  nux::Object* nux_object = NULL;
  FilterBasicButton* button = NULL;

  ATK_OBJECT_CLASS(unity_filter_basic_button_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_TOGGLE_BUTTON;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));

  if (nux_object == NULL) /* defunct */
    return;

  button = dynamic_cast<FilterBasicButton*>(nux_object);

  if (button == NULL) /* defunct */
    return;

  button->LayoutAdded.connect(sigc::bind(sigc::ptr_fun(on_layout_changed_cb),
                                       accessible, TRUE));

  button->key_nav_focus_change.connect(sigc::bind(sigc::ptr_fun(on_focus_changed_cb), accessible));
}

static const gchar*
unity_filter_basic_button_accessible_get_name(AtkObject* obj)
{
  const gchar* name;

  g_return_val_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(obj), NULL);

  name = ATK_OBJECT_CLASS(unity_filter_basic_button_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    FilterBasicButton* button = NULL;

    button = dynamic_cast<FilterBasicButton*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));

    if (button == NULL) /* State is defunct */
      name = NULL;
    else
      name = button->GetLabel().c_str();
  }

  if (name == NULL)
  {
    LOG_WARN(logger) << "Name == NULL";
  }

  return name;
}

static AtkStateSet*
unity_filter_basic_button_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;
  FilterBasicButton* button = NULL;

  g_return_val_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(unity_filter_basic_button_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  button = static_cast<FilterBasicButton*>(nux_object);

  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
  atk_state_set_add_state(state_set, ATK_STATE_SHOWING);

  if (button->GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
  {
    atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
    atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
    atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);
  }

  if (button->Active())
    atk_state_set_add_state(state_set, ATK_STATE_CHECKED);

  return state_set;
}

/* AtkAction */
static void
atk_action_interface_init(AtkActionIface *iface)
{
  iface->do_action = unity_filter_basic_button_accessible_do_action;
  iface->get_n_actions = unity_filter_basic_button_accessible_get_n_actions;
  iface->get_name = unity_filter_basic_button_accessible_get_action_name;
}

static gboolean
unity_filter_basic_button_accessible_do_action(AtkAction *action,
                                         gint i)
{
  FilterBasicButton* button = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(action), FALSE);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(action));
  if (nux_object == NULL)
    return FALSE;

  button = static_cast<FilterBasicButton*>(nux_object);
  button->Activate();

  return TRUE;
}

static gint
unity_filter_basic_button_accessible_get_n_actions(AtkAction *action)
{
  g_return_val_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(action), 0);

  return 1;
}

static const gchar*
unity_filter_basic_button_accessible_get_action_name(AtkAction *action,
                                        gint i)
{
  g_return_val_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(action), NULL);
  g_return_val_if_fail(i == 0, NULL);

  return "activate";
}

static void
on_layout_changed_cb(nux::View* view,
                     nux::Layout* layout,
                     AtkObject* accessible,
                     gboolean is_add)
{
  g_return_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(accessible));

  g_object_notify(G_OBJECT(accessible), "accessible-name");
}

static void
on_focus_changed_cb(nux::Area* area,
                    bool has_focus,
                    nux::KeyNavDirection direction,
                    AtkObject* accessible)
{
  g_return_if_fail(UNITY_IS_FILTER_BASIC_BUTTON_ACCESSIBLE(accessible));

  g_signal_emit_by_name(accessible, "focus-event", has_focus);
}
