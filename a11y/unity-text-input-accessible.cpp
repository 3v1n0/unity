/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "unity-text-input-accessible.h"

#include "unitya11y.h"
#include "TextInput.h"

using namespace unity;

/* GObject */
static void unity_text_input_accessible_class_init(UnityTextInputAccessibleClass* klass);
static void unity_text_input_accessible_init(UnityTextInputAccessible* self);
//static void unity_text_input_accessible_finalize(GObject* object);

/* AtkObject.h */
static void unity_text_input_accessible_initialize(AtkObject* accessible,
                                                   gpointer   data);

G_DEFINE_TYPE(UnityTextInputAccessible, unity_text_input_accessible,  NUX_TYPE_VIEW_ACCESSIBLE)

static void
unity_text_input_accessible_class_init(UnityTextInputAccessibleClass* klass)
{
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->initialize = unity_text_input_accessible_initialize;
}

static void
unity_text_input_accessible_init(UnityTextInputAccessible* self)
{}

AtkObject*
unity_text_input_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<TextInput*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(UNITY_TYPE_TEXT_INPUT_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

static void
unity_text_input_accessible_initialize(AtkObject* accessible,
                                       gpointer data)
{
  nux::Object* nux_object = NULL;
  TextInput* text_input = NULL;
  nux::TextEntry* text_entry = NULL;

  ATK_OBJECT_CLASS(unity_text_input_accessible_parent_class)->initialize(accessible, data);

  accessible->role = ATK_ROLE_PANEL;

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  text_input = dynamic_cast<TextInput*>(nux_object);

  if (text_input == NULL)
    return;

  text_entry = text_input->text_entry();

  if (text_entry != NULL)
  {
    AtkObject* text_entry_accessible = NULL;
    text_entry_accessible = unity_a11y_get_accessible(text_entry);
    atk_object_set_name(text_entry_accessible, text_input->input_hint().c_str());
  }
}
