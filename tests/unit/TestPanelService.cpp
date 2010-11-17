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

//------------------------ USEFUL FUNCTIONS -----------------------------------

guint
get_n_indicators_in_result (GVariant *result)
{
  guint  ret = 0;
  gchar *current_object_id = NULL;
  GVariantIter *iter;
  gchar        *indicator_id;
  gchar        *entry_id;
  gchar        *label;
  gboolean      label_sensitive;
  gboolean      label_visible;
  guint32       image_type;
  gchar        *image_data;
  gboolean      image_sensitive;
  gboolean      image_visible;

  g_variant_get (result, "(a(sssbbusbb))", &iter);
  while (g_variant_iter_loop (iter, "(sssbbusbb)",
                              &indicator_id,
                              &entry_id,
                              &label,
                              &label_sensitive,
                              &label_visible,
                              &image_type,
                              &image_data,
                              &image_sensitive,
                              &image_visible))
    {
      if (g_strcmp0 (current_object_id, indicator_id) != 0)
        {
          ret++;
          g_free (current_object_id);
          current_object_id = g_strdup (indicator_id);
        }
    }
  g_free (current_object_id);

  return ret;
}

//------------------------ /USEFUL FUNCTIONS ----------------------------------

static void TestAllocation  (void);
static void TestIndicatorLoading (void);
static void TestEmptyObjectMessage (void);

void
TestPanelServiceCreateSuite ()
{
#define _DOMAIN "/Unit/PanelService"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
  g_test_add_func (_DOMAIN"/IndicatorLoading", TestIndicatorLoading);
  g_test_add_func (_DOMAIN"/EmptyObjectMessage", TestEmptyObjectMessage);
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
TestIndicatorLoading ()
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
  g_object_unref (service);
}

static void
TestEmptyObjectMessage ()
{
  PanelService    *service;
  IndicatorObject *object;
  GList           *objects = NULL;
  GVariant        *result;

  object = test_object_new ();
  g_assert (INDICATOR_IS_OBJECT (object));
  objects = g_list_append (objects, object);

  service = panel_service_get_default_with_indicators (objects);
  g_assert (PANEL_IS_SERVICE (service));

  g_assert_cmpint (panel_service_get_n_indicators (service), ==, 1);

  result = panel_service_sync (service);
  g_assert (result != NULL);

  g_assert_cmpint (get_n_indicators_in_result (result), ==, 1);

  g_variant_unref (result);
  g_list_free (objects);
  g_object_unref (object);
}
