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
 * SECTION:nux-result-accessible
 * @Title: UnityResultAccessible
 * @short_description: Implementation of the ATK interfaces for a #Result
 * @see_also: unity::dash::Result
 *
 * #UnityResultAccessible implements the required ATK interfaces of
 * nux::Result, in order to represent each one of the elements of a
 * ResultGrid.
 *
 * The idea is having it as a fly-weight object. Note: it represents
 * it, but it doesn't maintain a reference to it.
 *
 */

#include "unity-result-accessible.h"
#include "unity-rvgrid-accessible.h"

#include "unitya11y.h"

/* GObject */
static void unity_result_accessible_class_init(UnityResultAccessibleClass* klass);
static void unity_result_accessible_init(UnityResultAccessible* result_accessible);

/* AtkObject.h */
static void          unity_result_accessible_initialize(AtkObject* accessible,
                                                        gpointer   data);
static AtkStateSet*  unity_result_accessible_ref_state_set(AtkObject* obj);

G_DEFINE_TYPE(UnityResultAccessible,
              unity_result_accessible,
              ATK_TYPE_OBJECT);

#define UNITY_RESULT_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_RESULT_ACCESSIBLE, \
                                UnityResultAccessiblePrivate))

struct _UnityResultAccessiblePrivate
{
};

static void
unity_result_accessible_class_init(UnityResultAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = unity_result_accessible_initialize;
  atk_class->ref_state_set = unity_result_accessible_ref_state_set;

  g_type_class_add_private(gobject_class, sizeof(UnityResultAccessiblePrivate));
}

static void
unity_result_accessible_init(UnityResultAccessible* result_accessible)
{
  UnityResultAccessiblePrivate* priv =
    UNITY_RESULT_ACCESSIBLE_GET_PRIVATE(result_accessible);

  result_accessible->priv = priv;
}

AtkObject*
unity_result_accessible_new()
{
  AtkObject* accessible = NULL;

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_RESULT_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, NULL);

  return accessible;
}

/* AtkObject.h */
static void
unity_result_accessible_initialize(AtkObject* accessible,
                                   gpointer data)
{
  ATK_OBJECT_CLASS(unity_result_accessible_parent_class)->initialize(accessible, data);

  /* On unity Result is just data, but on the accessible
     implementation we are using this object to represent each icon
     selected on the result grid, so a push button */
  atk_object_set_role(accessible, ATK_ROLE_PUSH_BUTTON);
}

static AtkStateSet*
unity_result_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;

  g_return_val_if_fail(UNITY_IS_RESULT_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(unity_result_accessible_parent_class)->ref_state_set(obj);

  /* by default, this is a fly-weight/dummy object, so if created we have
     this information */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);

  atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
  atk_state_set_add_state(state_set, ATK_STATE_SHOWING);

  /* This object is not focused, the focused is the parent
     ResultViewGrid */
  // atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
  atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
  atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);

  return state_set;
}
