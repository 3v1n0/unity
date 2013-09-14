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
 * SECTION:unity-sctext-accessible
 * @Title: UnitySctextAccessible
 * @short_description: Implementation of the ATK interfaces for #StaticCairoText
 * @see_also: StaticCairoText
 *
 * #UnitySctextAccessible implements the required ATK interfaces for
 * #StaticCairoText, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "unity-sctext-accessible.h"

#include "unitya11y.h"
#include "StaticCairoText.h"

/* GObject */
static void unity_sctext_accessible_class_init(UnitySctextAccessibleClass* klass);
static void unity_sctext_accessible_init(UnitySctextAccessible* self);

/* AtkObject.h */
static void         unity_sctext_accessible_initialize(AtkObject* accessible,
                                                       gpointer   data);
static const gchar* unity_sctext_accessible_get_name(AtkObject* obj);

G_DEFINE_TYPE(UnitySctextAccessible, unity_sctext_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


#define UNITY_SCTEXT_ACCESSIBLE_GET_PRIVATE(obj)                      \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), UNITY_TYPE_SCTEXT_ACCESSIBLE,  \
                                UnitySctextAccessiblePrivate))

struct _UnitySctextAccessiblePrivate
{
  gchar* stripped_name;
};


static void
unity_sctext_accessible_class_init(UnitySctextAccessibleClass* klass)
{
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->get_name = unity_sctext_accessible_get_name;
  atk_class->initialize = unity_sctext_accessible_initialize;

  g_type_class_add_private(gobject_class, sizeof(UnitySctextAccessiblePrivate));
}

static void
unity_sctext_accessible_init(UnitySctextAccessible* self)
{
  UnitySctextAccessiblePrivate* priv =
    UNITY_SCTEXT_ACCESSIBLE_GET_PRIVATE(self);

  self->priv = priv;
  priv->stripped_name = NULL;
}

AtkObject*
unity_sctext_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<unity::StaticCairoText*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_SCTEXT_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
on_label_text_change_cb(unity::StaticCairoText* label, UnitySctextAccessible* self)
{
  g_object_notify(G_OBJECT(self), "accessible-name");
}

static void
unity_sctext_accessible_initialize(AtkObject* accessible,
                                   gpointer data)
{
  nux::Object* nux_object = NULL;
  unity::StaticCairoText* label = NULL;

  ATK_OBJECT_CLASS(unity_sctext_accessible_parent_class)->initialize(accessible, data);

  atk_object_set_role(accessible, ATK_ROLE_LABEL);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  label = dynamic_cast<unity::StaticCairoText*>(nux_object);

  if (label == NULL) /* status defunct */
    return;

  label->sigTextChanged.connect(sigc::bind(sigc::ptr_fun(on_label_text_change_cb),
                                           UNITY_SCTEXT_ACCESSIBLE(accessible)));
}

static const gchar*
unity_sctext_accessible_get_name(AtkObject* obj)
{
  const gchar* name = NULL;
  UnitySctextAccessible* self = NULL;

  g_return_val_if_fail(UNITY_IS_SCTEXT_ACCESSIBLE(obj), NULL);
  self = UNITY_SCTEXT_ACCESSIBLE(obj);

  name = ATK_OBJECT_CLASS(unity_sctext_accessible_parent_class)->get_name(obj);
  if (name == NULL)
  {
    unity::StaticCairoText* text = NULL;

    if (self->priv->stripped_name != NULL)
    {
      g_free(self->priv->stripped_name);
      self->priv->stripped_name = NULL;
    }

    text = dynamic_cast<unity::StaticCairoText*>(nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj)));
    if (text != NULL)
    {
      name = text->GetText().c_str();
      pango_parse_markup(name, -1, 0, NULL,
                         &self->priv->stripped_name,
                         NULL, NULL);
      name = self->priv->stripped_name;
    }
  }

  return name;
}
