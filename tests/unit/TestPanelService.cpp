/*
 * Copyright 2010 Canonical Ltd.
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
 *
 */


#include "config.h"

#include "panel-service.h"
#include <libindicator/indicator-object.h>

//----------------------- TESTING INDICATOR STUFF -----------------------------

#define TEST_TYPE_OBJECT (test_object_get_type ())

#define TEST_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TEST_TYPE_OBJECT, TestObject))

#define TEST_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TEST_TYPE_OBJECT, TestObjectClass))

#define TEST_IS_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TEST_TYPE_OBJECT))

#define TEST_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TEST_TYPE_OBJECT))

#define TEST_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TEST_TYPE_OBJECT, TestObjectClass))

typedef struct _TestObject        TestObject;
typedef struct _TestObjectClass   TestObjectClass;

struct _TestObject
{
  IndicatorObject parent;
};

struct _TestObjectClass
{
  IndicatorObjectClass parent_class;
};

GType             test_object_get_type (void) G_GNUC_CONST;

IndicatorObject * test_object_new        ();

G_DEFINE_TYPE (TestObject, test_object, INDICATOR_OBJECT_TYPE);

GList *
test_object_get_entries (IndicatorObject *io)
{
  return NULL;
}

guint
test_object_get_location (IndicatorObject      *io,
                          IndicatorObjectEntry *entry)
{
  return 0;
}

void
test_object_entry_activate (IndicatorObject      *io,
                            IndicatorObjectEntry *entry,
                            guint                 timestamp)
{

}

void
test_object_class_init (TestObjectClass *klass)
{
  IndicatorObjectClass *ind_class = INDICATOR_OBJECT_CLASS (klass);

  ind_class->get_entries    = test_object_get_entries;
  ind_class->get_location   = test_object_get_location;
  ind_class->entry_activate = test_object_entry_activate;
}

void
test_object_init (TestObject *self)
{

}

IndicatorObject *
test_object_new ()
{
  return (IndicatorObject *)g_object_new (TEST_TYPE_OBJECT, NULL);
}

//----------------------- /TESTING INDICATOR STUFF ----------------------------

static void TestAllocation  (void);
static void TestEmptyObject (void);

void
TestPanelServiceCreateSuite ()
{
#define _DOMAIN "/Unit/PanelService"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
  g_test_add_func (_DOMAIN"/EmptyObject", TestEmptyObject);
}

static void
TestAllocation ()
{
  PanelService *service;

  service = panel_service_get_default ();
  g_assert (PANEL_IS_SERVICE (service));

  g_object_unref (service);
  g_assert (PANEL_IS_SERVICE (service) == FALSE);
}

static void
TestEmptyObject ()
{
  PanelService    *service;
  IndicatorObject *object;
  GList           *objects = NULL;

  object = test_object_new ();
  g_assert (INDICATOR_IS_OBJECT (object));
  objects = g_list_append (objects, object);

  service = panel_service_get_default_with_indicators (objects);
  g_assert (PANEL_IS_SERVICE (service));

  g_assert_cmpint (panel_service_get_n_indicators (service), ==, 1);

  g_list_free (objects);
  g_object_unref (object);
}
