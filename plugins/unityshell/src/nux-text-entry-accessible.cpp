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
 * SECTION:nux-text_entry-accessible
 * @Title: NuxTextEntryAccessible
 * @short_description: Implementation of the ATK interfaces for #TextEntry
 * @see_also: nux::TextEntry
 *
 * #NuxTextEntryAccessible implements the required ATK interfaces for
 * #StaticCairoText, mainly exposing the text as his name, as this
 * #object is mainly used as a label
 *
 */

#include <glib/gi18n.h>

#include "nux-text-entry-accessible.h"

#include "unitya11y.h"
#include <Nux/TextEntry.h>

/* GObject */
static void nux_text_entry_accessible_class_init(NuxTextEntryAccessibleClass* klass);
static void nux_text_entry_accessible_init(NuxTextEntryAccessible* self);

/* AtkObject.h */
static void         nux_text_entry_accessible_initialize(AtkObject* accessible,
                                                         gpointer   data);
static AtkStateSet* nux_text_entry_accessible_ref_state_set(AtkObject* obj);

/* Fixme: it should implement AtkText/AtkTextEditable interfaces */
G_DEFINE_TYPE(NuxTextEntryAccessible, nux_text_entry_accessible,  NUX_TYPE_VIEW_ACCESSIBLE);


static void
nux_text_entry_accessible_class_init(NuxTextEntryAccessibleClass* klass)
{
  AtkObjectClass* atk_class = ATK_OBJECT_CLASS(klass);

  /* AtkObject */
  atk_class->ref_state_set = nux_text_entry_accessible_ref_state_set;
  atk_class->initialize = nux_text_entry_accessible_initialize;
}

static void
nux_text_entry_accessible_init(NuxTextEntryAccessible* self)
{}

AtkObject*
nux_text_entry_accessible_new(nux::Object* object)
{
  AtkObject* accessible = NULL;

  g_return_val_if_fail(dynamic_cast<nux::TextEntry*>(object), NULL);

  accessible = ATK_OBJECT(g_object_new(NUX_TYPE_TEXT_ENTRY_ACCESSIBLE, NULL));

  atk_object_initialize(accessible, object);

  return accessible;
}

/* AtkObject.h */
static void
nux_text_entry_accessible_initialize(AtkObject* accessible,
                                     gpointer data)
{
  nux::Object* nux_object = NULL;
  nux::TextEntry* text_entry = NULL;

  ATK_OBJECT_CLASS(nux_text_entry_accessible_parent_class)->initialize(accessible, data);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(accessible));
  text_entry = dynamic_cast<nux::TextEntry*>(nux_object);

  atk_object_set_role(accessible, text_entry->PasswordMode() ? ATK_ROLE_PASSWORD_TEXT : ATK_ROLE_ENTRY);
}

static AtkStateSet*
nux_text_entry_accessible_ref_state_set(AtkObject* obj)
{
  AtkStateSet* state_set = NULL;
  nux::Object* nux_object = NULL;

  g_return_val_if_fail(NUX_IS_TEXT_ENTRY_ACCESSIBLE(obj), NULL);

  state_set = ATK_OBJECT_CLASS(nux_text_entry_accessible_parent_class)->ref_state_set(obj);

  nux_object = nux_object_accessible_get_object(NUX_OBJECT_ACCESSIBLE(obj));
  if (nux_object == NULL) /* defunct */
    return state_set;

  /* Text entry is editable by default */
  atk_state_set_add_state(state_set, ATK_STATE_EDITABLE);

  return state_set;
}
