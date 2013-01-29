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
 * SECTION:unity-rvgrid-accessible
 * @Title: UnityRvgridAccessible
 * @short_description: Implementation of the ATK interfaces for #ResultViewGrid
 * @see_also: Rvgrid
 *
 * #UnityRvgridAccessible implements the required ATK interfaces for
 * #Rvgrid, ie: exposing the different RvgridIcon on the model as
 * #child of the object.
 *
 */

#include <glib/gi18n.h>

#include "unity-rvgrid-accessible.h"
#include "unity-result-accessible.h"
#include "unity-places-group-accessible.h"

#include "unitya11y.h"
#include "ResultViewGrid.h"
#include "PlacesGroup.h"

using namespace unity::dash;

/* GObject */
static void unity_rvgrid_accessible_class_init(UnityRvgridAccessibleClass* klass);
static void unity_rvgrid_accessible_init(UnityRvgridAccessible* self);
static void unity_rvgrid_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_rvgrid_accessible_initialize(AtkObject* accessible,
                                                     gpointer   data);
static AtkStateSet*  unity_rvgrid_accessible_ref_state_set(AtkObject* obj);
static gint       unity_rvgrid_accessible_get_n_children(AtkObject* obj);
static AtkObject* unity_rvgrid_accessible_ref_child(AtkObject* obj,
                                                    gint i);

/* AtkSelection */
static void       atk_selection_interface_init(AtkSelectionIface* iface);
static AtkObject* unity_rvgrid_accessible_ref_selection(AtkSelection* selection,
                                                        gint i);
static gint       unity_rvgrid_accessible_get_selection_count(AtkSelection* selection);
static gboolean   unity_rvgrid_accessible_is_child_selected(AtkSelection* selection,
                                                            gint i);

/* private */

G_DEFINE_TYPE_WITH_CODE(UnityRvgridAccessible, unity_rvgrid_accessible,  NUX_TYPE_VIEW_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_SELECTION, atk_selection_interface_init))

#define UNITY_RVGRID_ACCESSIBLE_GET_PRIVATE(obj)                        \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_RVGRID_ACCESSIBLE,    \
                                UnityRvgridAccessiblePrivate))

struct _UnityRvgridAccessiblePrivate
{
  sigc::connection on_selection_change_connection;

  /* dummy selected result object */
  UnityResultAccessible* result;
  gboolean has_selection;
  gboolean focused;
};


static void
unity_rvgrid_accessible_class_init(UnityRvgridAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* GObject */
  gobject_class->finalize = unity_rvgrid_accessible_finalize;

  /* AtkObject */
  atk_class->get_n_children = unity_rvgrid_accessible_get_n_children;
  atk_class->ref_child = unity_rvgrid_accessible_ref_child;
  atk_class->initialize = unity_rvgrid_accessible_initialize;
  atk_class->ref_state_set = unity_rvgrid_accessible_ref_state_set;

  g_type_class_add_private(gobject_class, sizeof(UnityRvgridAccessiblePrivate));
}

static void
unity_rvgrid_accessible_init(UnityRvgridAccessible* self)
{
  UnityRvgridAccessiblePrivate* priv =
    UNITY_RVGRID_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_rvgrid_accessible_finalize(GObject* object)
{
  UnityRvgridAccessible* self = UNITY_RVGRID_ACCESSIBLE(object);

  self->priv->on_selection_change_connection.disconnect();

  if (self->priv->result != NULL)
  {
    g_object_unref(self->priv->result);
    self->priv->result = NULL;
  }

  self->priv->on_selection_change_connection.disconnect();

  G_OBJECT_CLASS(unity_rvgrid_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_rvgrid_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<ResultViewGrid*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_RVGRID_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
check_selection(UnityRvgridAccessible* self)
{
  AtkObject* child = NULL;
  gint index = 0;
  nux::Object* object = NULL;
  ResultViewGrid* rvgrid = NULL;
  std::string name;

  /* we don't notify until the grid is focused */
  if (self->priv->focused == FALSE)
    return;

  object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  if (!object) /* state is defunct */
    return;

  rvgrid = dynamic_cast<ResultViewGrid*>(object);

  index = rvgrid->GetSelectedIndex();

  if (index >= 0)
  {
    Result result(*rvgrid->GetIteratorAtRow(index));
    name = result.name;

    child = ATK_OBJECT(self->priv->result);
    self->priv->has_selection = TRUE;
    atk_object_set_name(child, name.c_str());
  }
  else
  {
    child = NULL;
    self->priv->has_selection = FALSE;
  }

  g_signal_emit_by_name(self, "active-descendant-changed", child);
  g_signal_emit_by_name(self, "selection-changed");
}

static void
on_selection_change_cb(UnityRvgridAccessible* self)
{
  check_selection(self);
}

static void
search_for_label(UnityRvgridAccessible* self)
{
  AtkObject* label_accessible = NULL;
  nux::Object* nux_object = NULL;
  unity::dash::PlacesGroup* group = NULL;
  AtkObject* iter = NULL;
  unity::StaticCairoText* label = NULL;

  /* Search for the places group */
  for (iter = atk_object_get_parent(ATK_OBJECT(self)); iter != NULL;
       iter = atk_object_get_parent(iter))
  {
    if (UNITY_IS_PLACES_GROUP_ACCESSIBLE(iter))
      break;
  }
  if (iter == NULL)
    return;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(iter));
  group = dynamic_cast<unity::dash::PlacesGroup*>(nux_object);

  if (group == NULL)
    return;

  label = group->GetLabel();

  label_accessible = unity_a11y_get_accessible(label);

  if (label_accessible == NULL)
    return;

  /* FIXME: I had a froze using relations, require further
     investigation, meanwhile setting directly the name can do the
     work*/
  atk_object_set_name(ATK_OBJECT(self), atk_object_get_name(label_accessible));
}

static gboolean
check_selection_on_idle(gpointer data)
{
  check_selection(UNITY_RVGRID_ACCESSIBLE(data));

  return FALSE;
}

static void
on_focus_event_cb(AtkObject* object,
                  gboolean in,
                  gpointer data)
{
  UnityRvgridAccessible* self = NULL;

  g_return_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(data));

  self = UNITY_RVGRID_ACCESSIBLE(data);
  self->priv->focused = in;

  /* we check the selection stuff again, to report the selection
     change now */
  g_idle_add(check_selection_on_idle, self);
}

static void
unity_rvgrid_accessible_initialize(AtkObject* accessible,
                                   gpointer data)
{
  UnityRvgridAccessible* self = NULL;
  ResultViewGrid* rvgrid = NULL;
  nux::Object* nux_object = NULL;

  ATK_OBJECT_CLASS(unity_rvgrid_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role(accessible, ATK_ROLE_TOOL_BAR);

  self = UNITY_RVGRID_ACCESSIBLE(accessible);
  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));

  rvgrid = dynamic_cast<ResultViewGrid*>(nux_object);

  if (rvgrid == NULL)
    return;

  self->priv->on_selection_change_connection =
    rvgrid->selection_change.connect(sigc::bind(sigc::ptr_fun(on_selection_change_cb), self));

  g_signal_connect(self, "focus-event",
                   G_CALLBACK(on_focus_event_cb), self);

  self->priv->result = UNITY_RESULT_ACCESSIBLE(unity_result_accessible_new());
  atk_object_set_parent(ATK_OBJECT(self->priv->result), ATK_OBJECT(self));

  search_for_label(self);
}

static gint
unity_rvgrid_accessible_get_n_children(AtkObject* obj)
{
  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(obj), 0);

  /* we have the state MANAGES_DESCENDANT, clients should not ask for
     the children, and just taking care of the relevant signals. So we
     just don't expose the children */
  return 0;
}

static AtkObject*
unity_rvgrid_accessible_ref_child(AtkObject* obj,
                                  gint i)
{
  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(obj), NULL);

  /* we have the state MANAGES_DESCENDANT, clients should not ask for
     the children, and just taking care of the relevant signals. So we
     just don't expose the children */
  return NULL;
}

static AtkStateSet*
unity_rvgrid_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(unity_rvgrid_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));

  if (nux_object == NULL) /* defunct */
    return state_set;

  atk_state_set_add_state(state_set, ATK_STATE_MANAGES_DESCENDANTS);

  return state_set;
}


/* AtkSelection */
static void
atk_selection_interface_init(AtkSelectionIface* iface)
{
  iface->ref_selection = unity_rvgrid_accessible_ref_selection;
  iface->get_selection_count = unity_rvgrid_accessible_get_selection_count;
  iface->is_child_selected = unity_rvgrid_accessible_is_child_selected;

  /* NOTE: for the moment we don't provide the implementation for the
     "interactable" methods, it is, the methods that allow to change
     the selected icon. The Rvgrid doesn't provide that API, and
     right now  we are focusing on a normal user input.*/
  /* iface->add_selection = unity_rvgrid_accessible_add_selection; */
  /* iface->clear_selection = unity_rvgrid_accessible_clear_selection; */
  /* iface->remove_selection = unity_rvgrid_accessible_remove_selection; */

  /* This method will never be implemented, as select all the rvgrid
     icons makes no sense */
  /* iface->select_all = unity_rvgrid_accessible_select_all_selection; */
}

static AtkObject*
unity_rvgrid_accessible_ref_selection(AtkSelection* selection,
                                      gint i)
{
  UnityRvgridAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(selection), NULL);

  self = UNITY_RVGRID_ACCESSIBLE(selection);

  if (self->priv->has_selection)
    return ATK_OBJECT(g_object_ref(self->priv->result));
  else
    return NULL;
}

static gint
unity_rvgrid_accessible_get_selection_count(AtkSelection* selection)
{
  UnityRvgridAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(selection), 0);

  self = UNITY_RVGRID_ACCESSIBLE(selection);

  if (self->priv->has_selection)
    return 0;
  else
    return 1;
}

static gboolean
unity_rvgrid_accessible_is_child_selected(AtkSelection* selection,
                                          gint i)
{
  UnityRvgridAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(selection), FALSE);

  self = UNITY_RVGRID_ACCESSIBLE(selection);

  if (self->priv->has_selection && i == 0)
    return TRUE;
  else
    return FALSE;
}
