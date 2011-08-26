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

#include "unitya11y.h"
#include "ResultViewGrid.h"

using namespace unity::dash;

/* GObject */
static void unity_rvgrid_accessible_class_init(UnityRvgridAccessibleClass* klass);
static void unity_rvgrid_accessible_init(UnityRvgridAccessible* self);
static void unity_rvgrid_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_rvgrid_accessible_initialize(AtkObject* accessible,
                                                     gpointer   data);
static gint       unity_rvgrid_accessible_get_n_children(AtkObject* obj);
static AtkObject* unity_rvgrid_accessible_ref_child(AtkObject* obj,
                                                    gint i);

/* AtkSelection */
static void       atk_selection_interface_init(AtkSelectionIface* iface);

/* private */

G_DEFINE_TYPE_WITH_CODE(UnityRvgridAccessible, unity_rvgrid_accessible,  NUX_TYPE_VIEW_ACCESSIBLE,
                        G_IMPLEMENT_INTERFACE(ATK_TYPE_SELECTION, atk_selection_interface_init))

#define UNITY_RVGRID_ACCESSIBLE_GET_PRIVATE(obj)                        \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_RVGRID_ACCESSIBLE,    \
                                UnityRvgridAccessiblePrivate))

struct _UnityRvgridAccessiblePrivate
{
  sigc::connection on_selection_change_connection;
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
  UnityRvgridAccessible* self = UNITY_RVGRID_ACCESSIBLE (object);

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
static void on_selection_change_cb(UnityRvgridAccessible* self)
{
  g_debug ("[RVGrid] selection change!!");
}

static void
unity_rvgrid_accessible_initialize(AtkObject* accessible,
                                   gpointer data)
{
  UnityRvgridAccessible* self = NULL;
  ResultViewGrid* rvgrid = NULL;
  nux::Object* nux_object = NULL;

  ATK_OBJECT_CLASS(unity_rvgrid_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role (accessible, ATK_ROLE_TOOL_BAR);

  self = UNITY_RVGRID_ACCESSIBLE(accessible);
  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));

  rvgrid = dynamic_cast<ResultViewGrid*>(nux_object);

  if (rvgrid == NULL)
    return;

  self->priv->on_selection_change_connection =
    rvgrid->selection_change.connect(sigc::bind(sigc::ptr_fun(on_selection_change_cb), self));
}

static gint
unity_rvgrid_accessible_get_n_children(AtkObject* obj)
{
  nux::Object* object = NULL;
  ResultViewGrid* rvgrid = NULL;
  ResultView::ResultList result_list;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(obj), 0);

  object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!object) /* state is defunct */
    return 0;

  rvgrid = dynamic_cast<ResultViewGrid*>(object);

  result_list = rvgrid->GetResultList();

  return result_list.size();
}

static AtkObject*
unity_rvgrid_accessible_ref_child(AtkObject* obj,
                                  gint i)
{
  gint num = 0;
  nux::Object* nux_object = NULL;
  ResultViewGrid* rvgrid = NULL;

  g_return_val_if_fail(UNITY_IS_RVGRID_ACCESSIBLE(obj), NULL);
  num = atk_object_get_n_accessible_children(obj);
  g_return_val_if_fail((i < num) && (i >= 0), NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!nux_object) /* state is defunct */
    return 0;

  rvgrid = dynamic_cast<ResultViewGrid*>(nux_object);
  rvgrid->GetResultList();
  /* FIXME: implement me! */
  return NULL;
}

/* AtkSelection */
static void
atk_selection_interface_init(AtkSelectionIface* iface)
{
  // iface->ref_selection = unity_rvgrid_accessible_ref_selection;
  // iface->get_selection_count = unity_rvgrid_accessible_get_selection_count;
  // iface->is_child_selected = unity_rvgrid_accessible_is_child_selected;

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
