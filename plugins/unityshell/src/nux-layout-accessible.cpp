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
 * SECTION:nux-layout-accessible
 * @Title: NuxLayoutAccessible
 * @short_description: Implementation of the ATK interfaces for #nux::Layout
 * @see_also: nux::Layout
 *
 * #NuxLayoutAccessible implements the required ATK interfaces of
 * nux::Layout, implementing the container related methods on
 * AtkObject, in order to expose its objects
 *
 */

#include "nux-layout-accessible.h"

#include "unitya11y.h"

/* GObject */
static void nux_layout_accessible_class_init(NuxLayoutAccessibleClass* klass);
static void nux_layout_accessible_init(NuxLayoutAccessible* layout_accessible);

/* AtkObject.h */
static void       nux_layout_accessible_initialize(AtkObject* accessible,
                                                   gpointer   data);
static gint       nux_layout_accessible_get_n_children(AtkObject* obj);
static AtkObject* nux_layout_accessible_ref_child(AtkObject* obj,
                                                  gint i);

/* private */
static void       on_view_changed_cb(nux::Layout* layout,
                                     nux::Area* area,
                                     AtkObject* acccessible,
                                     gboolean is_add);
static int        search_for_child(AtkObject* accessible,
                                   nux::Layout* layout,
                                   nux::Area* area);

G_DEFINE_TYPE(NuxLayoutAccessible, nux_layout_accessible,  NUX_TYPE_AREA_ACCESSIBLE)

static void
nux_layout_accessible_class_init(NuxLayoutAccessibleClass* klass)
{
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = nux_layout_accessible_initialize;
  atk_class->ref_child = nux_layout_accessible_ref_child;
  atk_class->get_n_children = nux_layout_accessible_get_n_children;
}

static void
nux_layout_accessible_init(NuxLayoutAccessible* layout_accessible)
{
}

AtkObject*
nux_layout_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<nux::Layout*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(NUX_TYPE_LAYOUT_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_layout_accessible_initialize(AtkObject* accessible,
                                 gpointer data)
{
  nux::Object* nux_object = NULL;
  nux::Layout* layout = NULL;

  ATK_OBJECT_CLASS(nux_layout_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  layout = dynamic_cast<nux::Layout*>(nux_object);

  layout->ViewAdded.connect(sigc::bind(sigc::ptr_fun(on_view_changed_cb),
                                       accessible, TRUE));

  layout->ViewRemoved.connect(sigc::bind(sigc::ptr_fun(on_view_changed_cb),
                                         accessible, FALSE));
}

static gint
nux_layout_accessible_get_n_children(AtkObject* obj)
{
  nux::Object* nux_object = NULL;
  nux::Layout* layout = NULL;
  std::list<nux::Area*> element_list;

  g_return_val_if_fail(NUX_IS_LAYOUT_ACCESSIBLE(obj), 0);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!nux_object) /* state is defunct */
    return 0;

  layout = dynamic_cast<nux::Layout*>(nux_object);

  element_list = layout->GetChildren();

  return element_list.size();
}

static AtkObject*
nux_layout_accessible_ref_child(AtkObject* obj,
                                gint i)
{
  nux::Object* nux_object = NULL;
  nux::Object* child = NULL;
  AtkObject* child_accessible = NULL;
  nux::Layout* layout = NULL;
  std::list<nux::Area*> element_list;
  gint num = 0;
  std::list<nux::Area*>::iterator it;
  AtkObject* parent = NULL;

  g_return_val_if_fail(NUX_IS_LAYOUT_ACCESSIBLE(obj), 0);
  num = atk_object_get_n_accessible_children(obj);
  g_return_val_if_fail((i < num) && (i >= 0), NULL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (!nux_object) /* state is defunct */
    return 0;

  layout = dynamic_cast<nux::Layout*>(nux_object);

  element_list = layout->GetChildren();

  it = element_list.begin();
  std::advance(it, i);

  child = dynamic_cast<nux::Object*>(*it);
  child_accessible = unity_a11y_get_accessible(child);

  parent = atk_object_get_parent(child_accessible);
  if (parent != obj)
    atk_object_set_parent(child_accessible, obj);

  g_object_ref(child_accessible);

  return child_accessible;
}

/* private */
static void
on_view_changed_cb(nux::Layout* layout,
                   nux::Area* area,
                   AtkObject* accessible,
                   gboolean is_add)
{
  const gchar* signal_name = NULL;
  AtkObject* atk_child = NULL;
  gint index;

  g_return_if_fail(NUX_IS_LAYOUT_ACCESSIBLE(accessible));

  atk_child = unity_a11y_get_accessible(area);

  if (is_add)
  {
    signal_name = "children-changed::add";
    index = nux_layout_accessible_get_n_children(accessible) - 1;
  }
  else
  {
    signal_name = "children-changed::remove";
    index = search_for_child(accessible, layout, area);
  }

  g_signal_emit_by_name(accessible, signal_name, index, atk_child, NULL);
}

static int
search_for_child(AtkObject* accessible,
                 nux::Layout* layout,
                 nux::Area* area)
{
  std::list<nux::Area*> element_list;
  std::list<nux::Area*>::iterator it;
  nux::Area* current_area = NULL;
  gint result = 0;
  gboolean found = FALSE;

  element_list = layout->GetChildren();

  for (it = element_list.begin(); it != element_list.end(); ++it, result++)
  {
    current_area = *it;
    if (current_area == area)
    {
      found = TRUE;
      break;
    }
  }

  if (!found) result = -1;

  return result;
}
