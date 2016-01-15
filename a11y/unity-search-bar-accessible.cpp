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
 * SECTION:unity-search_bar-accessible
 * @Title: UnitySearchBarAccessible
 * @short_description: Implementation of the ATK interfaces for #SearchBar
 * @see_also: SearchBar at SearchBar.h
 *
 * #UnitySearchBarAccessible implements the required ATK interfaces for
 * #SearchBar, ie: exposing the different SearchBarIcon on the model as
 * #child of the object.
 *
 */

#include <glib/gi18n.h>

#include "unity-search-bar-accessible.h"

#include "unitya11y.h"
#include "SearchBar.h"

using namespace unity;

/* GObject */
static void unity_search_bar_accessible_class_init(UnitySearchBarAccessibleClass* klass);
static void unity_search_bar_accessible_init(UnitySearchBarAccessible* self);
static void unity_search_bar_accessible_finalize(GObject* object);

/* AtkObject.h */
static void       unity_search_bar_accessible_initialize(AtkObject* accessible,
                                                         gpointer   data);

G_DEFINE_TYPE(UnitySearchBarAccessible, unity_search_bar_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

#define UNITY_SEARCH_BAR_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_SEARCH_BAR_ACCESSIBLE,  \
                                UnitySearchBarAccessiblePrivate))

struct _UnitySearchBarAccessiblePrivate
{

};


static void
unity_search_bar_accessible_class_init(UnitySearchBarAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  gobject_class->finalize = unity_search_bar_accessible_finalize;

  /* AtkObject */
  atk_class->initialize = unity_search_bar_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnitySearchBarAccessiblePrivate));
}

static void
unity_search_bar_accessible_init(UnitySearchBarAccessible* self)
{
  UnitySearchBarAccessiblePrivate* priv =
    UNITY_SEARCH_BAR_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
}

static void
unity_search_bar_accessible_finalize(GObject* object)
{
  G_OBJECT_CLASS(unity_search_bar_accessible_parent_class)->finalize(object);
}

AtkObject*
unity_search_bar_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<SearchBar*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_SEARCH_BAR_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
on_search_hint_change_cb(std::string const& s, UnitySearchBarAccessible* self)
{
  SearchBar* search_bar = NULL;
  nux::TextEntry* text_entry = NULL;
  nux::Object* nux_object = NULL;

  g_return_if_fail(UNITY_IS_SEARCH_BAR_ACCESSIBLE(self));

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  search_bar = dynamic_cast<SearchBar*>(nux_object);

  if (search_bar == NULL) /* state defunct */
    return;

  text_entry = search_bar->text_entry();

  if (text_entry != NULL)
  {
    AtkObject* text_entry_accessible = NULL;
    text_entry_accessible = unity_a11y_get_accessible(text_entry);
    atk_object_set_name(text_entry_accessible, s.c_str());
  }
}


static void
unity_search_bar_accessible_initialize(AtkObject* accessible,
                                       gpointer data)
{
  nux::Object* nux_object = NULL;
  SearchBar* search_bar = NULL;
  // nux::TextEntry* text_entry = NULL;

  ATK_OBJECT_CLASS(unity_search_bar_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  search_bar = dynamic_cast<SearchBar*>(nux_object);

  if (search_bar == NULL)
    return;

  search_bar->search_hint.changed.connect(sigc::bind(sigc::ptr_fun(on_search_hint_change_cb),
                                                     UNITY_SEARCH_BAR_ACCESSIBLE(accessible)));
}
