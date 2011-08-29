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
 * The idea is having it as a fly-weight object
 *
 */

#include "unity-result-accessible.h"
#include "unity-rvgrid-accessible.h"

#include "unitya11y.h"

/* GObject */
static void unity_result_accessible_class_init(UnityResultAccessibleClass* klass);
static void unity_result_accessible_init(UnityResultAccessible* result_accessible);
static void unity_result_accessible_dispose(GObject* object);


/* AtkObject.h */
static void          unity_result_accessible_initialize(AtkObject* accessible,
                                                               gpointer   data);
static AtkStateSet*  unity_result_accessible_ref_state_set(AtkObject* obj);
static const gchar*  unity_result_accessible_get_name(AtkObject* obj);

/* private/utility methods*/
static void on_parent_selection_change_cb(AtkSelection* selection,
                                          gpointer data);
static void on_parent_focus_event_cb(AtkObject* object,
                                     gboolean in,
                                     gpointer data);

G_DEFINE_TYPE(UnityResultAccessible,
              unity_result_accessible,
              ATK_TYPE_OBJECT);

#define UNITY_RESULT_ACCESSIBLE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_RESULT_ACCESSIBLE, \
                                UnityResultAccessiblePrivate))

struct _UnityResultAccessiblePrivate
{
  unity::dash::Result *result;

  guint on_parent_selection_change_id;
  guint on_parent_focus_event_id;
};

static void
unity_result_accessible_class_init(UnityResultAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->dispose = unity_result_accessible_dispose;

  /* AtkObject */
  atk_class->initialize = unity_result_accessible_initialize;
  atk_class->get_name = unity_result_accessible_get_name;
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

static void
unity_result_accessible_dispose(GObject* object)
{
  UnityResultAccessible* self = UNITY_RESULT_ACCESSIBLE(object);
  AtkObject* parent = NULL;

  parent = atk_object_get_parent(ATK_OBJECT(object));

  if (UNITY_IS_RESULT_ACCESSIBLE(parent))
  {
    if (self->priv->on_parent_selection_change_id != 0)
      g_signal_handler_disconnect(parent, self->priv->on_parent_selection_change_id);

    if (self->priv->on_parent_focus_event_id != 0)
      g_signal_handler_disconnect(parent, self->priv->on_parent_focus_event_id);
  }

  G_OBJECT_CLASS(unity_result_accessible_parent_class)->dispose(object);
}


AtkObject*
unity_result_accessible_new(unity::dash::Result *result)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<unity::dash::Result*>(result), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_RESULT_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, result);

  return accessible;
}

/* AtkObject.h */
static void
unity_result_accessible_initialize(AtkObject* accessible,
                                   gpointer data)
{
  AtkObject* parent = NULL;
  UnityResultAccessible* self = NULL;

  ATK_OBJECT_CLASS(unity_result_accessible_parent_class)->initialize(accessible, data);

  /* On unity Result is just data, but on the accessible
     implementation we are using this object to represent each icon
     selected on the result grid */
  atk_object_set_role(accessible, ATK_ROLE_PUSH_BUTTON);

  /* FIXME: we need to connect to this signals somehow, at this moment
     parent will return NULL */
  parent = atk_object_get_parent (accessible);

  if (parent == NULL)
    return;

  self = UNITY_RESULT_ACCESSIBLE (accessible);

  self->priv->on_parent_selection_change_id =
    g_signal_connect(parent, "selection-changed",
                     G_CALLBACK(on_parent_selection_change_cb), self);

  self->priv->on_parent_focus_event_id =
    g_signal_connect(parent, "focus-event",
                     G_CALLBACK(on_parent_focus_event_cb), self);

  self->priv->result = (unity::dash::Result*)(data);
}


static const gchar*
unity_result_accessible_get_name(AtkObject* obj)
{
  const gchar* name;

  g_return_val_if_fail(UNITY_IS_RESULT_ACCESSIBLE(obj), NULL);

  name = ATK_OBJECT_CLASS(unity_result_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    std::string uri;
    UnityResultAccessible *self = NULL;

    self = UNITY_RESULT_ACCESSIBLE (obj);

    uri = self->priv->result->uri;

    name = uri.c_str ();
  }

  return name;
}

static AtkStateSet*
unity_result_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  // UnityResultAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_RESULT_ACCESSIBLE(obj), NULL);
  // self = UNITY_RESULT_ACCESSIBLE(obj);

  state_set = ATK_OBJECT_CLASS(unity_result_accessible_parent_class)->ref_state_set(obj);

  /* by default, this is a fly-weight object, so if created we have
     this information */
  atk_state_set_add_state(state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state(state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state(state_set, ATK_STATE_SENSITIVE);

  atk_state_set_add_state(state_set, ATK_STATE_VISIBLE);
  atk_state_set_add_state(state_set, ATK_STATE_SHOWING);

  atk_state_set_add_state(state_set, ATK_STATE_FOCUSED);
  atk_state_set_add_state(state_set, ATK_STATE_SELECTED);
  atk_state_set_add_state(state_set, ATK_STATE_ACTIVE);

  return state_set;
}

static void
on_parent_selection_change_cb(AtkSelection* selection,
                              gpointer data)
{
  g_return_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(selection));
  g_return_if_fail(UNITY_IS_RESULT_ACCESSIBLE(data));

  g_debug ("[ResultA %s] selection_change_cb", atk_object_get_name (ATK_OBJECT (data)));
}


static void
on_parent_focus_event_cb(AtkObject* object,
                         gboolean in,
                         gpointer data)
{
  g_return_if_fail(UNITY_IS_RESULT_ACCESSIBLE(data));

  /* we check the selection stuff again, to report the focus change
     now */
  g_debug ("[ResultA %s] on_parent_focus_event_cb", atk_object_get_name (ATK_OBJECT (data)));
}
