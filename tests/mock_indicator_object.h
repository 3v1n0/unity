/*
 * Copyright 2010-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#ifndef TESTS_MOCK_INDICATOR_OBJECT_H
#define TESTS_MOCK_INDICATOR_OBJECT_H

#include <libindicator/indicator-object.h>


#define MOCK_TYPE_INDICATOR_OBJECT (mock_indicator_object_get_type ())

#define MOCK_INDICATOR_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  MOCK_TYPE_INDICATOR_OBJECT, MockIndicatorObject))

#define MOCK_INDICATOR_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  MOCK_TYPE_INDICATOR_OBJECT, MockIndicatorObjectClass))

#define MOCK_IS_INDICATOR_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  MOCK_TYPE_INDICATOR_OBJECT))

#define MOCK_IS_INDICATOR_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  MOCK_TYPE_INDICATOR_OBJECT))

#define MOCK_INDICATOR_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  MOCK_TYPE_INDICATOR_OBJECT, MockIndicatorObjectClass))

typedef struct _MockIndicatorObject        MockIndicatorObject;
typedef struct _MockIndicatorObjectClass   MockIndicatorObjectClass;

struct _MockIndicatorObject
{
  IndicatorObject parent;

  GList* entries;
};

struct _MockIndicatorObjectClass
{
  IndicatorObjectClass parent_class;
};

GType mock_indicator_object_get_type(void) G_GNUC_CONST;

IndicatorObject * mock_indicator_object_new ();
IndicatorObjectEntry * mock_indicator_object_add_entry(MockIndicatorObject *self,
                                                       const gchar *label,
                                                       const gchar *icon_name);

//--- .c

G_DEFINE_TYPE(MockIndicatorObject, mock_indicator_object, INDICATOR_OBJECT_TYPE);

void
mock_indicator_object_dispose(GObject* object)
{
  MockIndicatorObject* self = MOCK_INDICATOR_OBJECT(object);
  g_list_free_full(self->entries, g_free);
  G_OBJECT_CLASS(mock_indicator_object_parent_class)->dispose(object);
}

GList*
mock_indicator_object_get_entries(IndicatorObject* io)
{
  g_return_val_if_fail(MOCK_IS_INDICATOR_OBJECT(io), NULL);

  return g_list_copy(MOCK_INDICATOR_OBJECT(io)->entries);
}

guint
mock_indicator_object_get_location(IndicatorObject* io, IndicatorObjectEntry* entry)
{
  g_return_val_if_fail(MOCK_IS_INDICATOR_OBJECT(io), 0);

  return g_list_index(MOCK_INDICATOR_OBJECT(io)->entries, entry);
}

void
mock_indicator_object_entry_activate(IndicatorObject* io, IndicatorObjectEntry* entry, guint timestamp)
{
}

void
mock_indicator_object_class_init(MockIndicatorObjectClass* klass)
{
  GObjectClass* obj_class = G_OBJECT_CLASS(klass);
  obj_class->dispose = mock_indicator_object_dispose;

  IndicatorObjectClass* ind_class = INDICATOR_OBJECT_CLASS(klass);
  ind_class->get_entries = mock_indicator_object_get_entries;
  ind_class->get_location = mock_indicator_object_get_location;
  ind_class->entry_activate = mock_indicator_object_entry_activate;
}

void
mock_indicator_object_init(MockIndicatorObject* self)
{}

IndicatorObject*
mock_indicator_object_new()
{
  return (IndicatorObject*) g_object_new(MOCK_TYPE_INDICATOR_OBJECT, NULL);
}

IndicatorObjectEntry*
mock_indicator_object_add_entry(MockIndicatorObject*  self, const gchar* label, const gchar* icon_name)
{
  IndicatorObjectEntry* entry;
  g_return_val_if_fail(MOCK_IS_INDICATOR_OBJECT(self), NULL);

  entry = g_new0(IndicatorObjectEntry, 1);
  entry->label = (GtkLabel*) gtk_label_new(label);
  entry->image = icon_name ? (GtkImage*) gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU) : NULL;
  entry->menu = NULL;

  self->entries = g_list_append(self->entries, entry);

  g_signal_emit(self, INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED_ID, 0, entry, TRUE);

  return entry;
}

void
mock_indicator_object_show_entry(MockIndicatorObject* self, IndicatorObjectEntry* entry, guint timestamp)
{
  g_signal_emit(self, INDICATOR_OBJECT_SIGNAL_MENU_SHOW_ID, 0, entry, timestamp);
}

#endif // TESTS_MOCK_INDICATOR_OBJECT_H
