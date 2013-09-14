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
 * SECTION:unity-places_group-accessible
 * @Title: UnityPlacesGroupAccessible
 * @short_description: Implementation of the ATK interfaces for #PlacesGroup
 * @see_also: PlacesGroup
 *
 * #UnityPlacesGroupAccessible implements the required ATK interfaces for
 * #PlacesGroup, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>

#include "unity-places-group-accessible.h"

#include "unitya11y.h"
#include "PlacesGroup.h"

/* GObject */
static void unity_places_group_accessible_class_init(UnityPlacesGroupAccessibleClass* klass);
static void unity_places_group_accessible_init(UnityPlacesGroupAccessible* self);

/* AtkObject.h */
static void         unity_places_group_accessible_initialize(AtkObject* accessible,
                                                             gpointer   data);

G_DEFINE_TYPE(UnityPlacesGroupAccessible, unity_places_group_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


#define UNITY_PLACES_GROUP_ACCESSIBLE_GET_PRIVATE(obj)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_PLACES_GROUP_ACCESSIBLE, \
                                UnityPlacesGroupAccessiblePrivate))

struct _UnityPlacesGroupAccessiblePrivate
{
  gchar* stripped_name;
};


static void
unity_places_group_accessible_class_init(UnityPlacesGroupAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = unity_places_group_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnityPlacesGroupAccessiblePrivate));
}

static void
unity_places_group_accessible_init(UnityPlacesGroupAccessible* self)
{
  UnityPlacesGroupAccessiblePrivate* priv =
    UNITY_PLACES_GROUP_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  priv->stripped_name = NULL;
}

AtkObject*
unity_places_group_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<unity::dash::PlacesGroup*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_PLACES_GROUP_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
/* expand label are usually focused during the key nav, but it don't
 * get a proper name always. In those cases we use the label.
 *
 * In the same way, it is possible that the PlacesGroup get focused
 * so we also set the own name with this label
 */
static void
ensure_proper_name(UnityPlacesGroupAccessible* self)
{
  unity::dash::PlacesGroup* group = NULL;
  nux::Object* nux_object = NULL;
  unity::StaticCairoText* label = NULL;
  unity::StaticCairoText* expand_label = NULL;
  AtkObject* label_accessible = NULL;
  AtkObject* expand_label_accessible = NULL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(self));
  group = dynamic_cast<unity::dash::PlacesGroup*>(nux_object);

  if (group == NULL)
    return;

  label = group->GetLabel();
  expand_label = group->GetExpandLabel();


  label_accessible = unity_a11y_get_accessible(label);
  expand_label_accessible = unity_a11y_get_accessible(expand_label);

  if ((label_accessible == NULL) || (expand_label_accessible == NULL))
    return;

  atk_object_set_name(ATK_OBJECT(self), atk_object_get_name(label_accessible));

  if (expand_label->GetText() == "")
    atk_object_set_name(expand_label_accessible, atk_object_get_name(label_accessible));
}


static void
on_label_text_change_cb(unity::StaticCairoText* label, UnityPlacesGroupAccessible* self)
{
  ensure_proper_name(self);
}

static void
unity_places_group_accessible_initialize(AtkObject* accessible,
                                         gpointer data)
{
  unity::dash::PlacesGroup* group = NULL;
  nux::Object* nux_object = NULL;
  unity::StaticCairoText* label = NULL;

  ATK_OBJECT_CLASS(unity_places_group_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role(accessible, ATK_ROLE_PANEL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  group = dynamic_cast<unity::dash::PlacesGroup*>(nux_object);

  if (group == NULL)
    return;

  label = group->GetLabel();

  if (label == NULL)
    return;

  ensure_proper_name(UNITY_PLACES_GROUP_ACCESSIBLE(accessible));
  label->sigTextChanged.connect(sigc::bind(sigc::ptr_fun(on_label_text_change_cb),
                                           UNITY_PLACES_GROUP_ACCESSIBLE(accessible)));
}

