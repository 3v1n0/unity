/*
 * GObject Class to allow simple gobject testing
 */

#ifndef _TEST_GOBJECT_H_
#define _TEST_GOBJECT_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define TEST_TYPE_GOBJECT (test_gobject_get_type ())

#define TEST_GOBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
  TEST_TYPE_GOBJECT, TestGObject))

#define TEST_GOBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
  TEST_TYPE_GOBJECT, TestGObjectClass))

#define TEST_IS_GOBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
  TEST_TYPE_GOBJECT))

#define TEST_IS_GOBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
  TEST_TYPE_GOBJECT))

#define TEST_GOBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
  TEST_TYPE_GOBJECT, TestGObjectClass))

#define TEST_GOBJECT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
  TEST_TYPE_GOBJECT, TestGObjectPrivate))

typedef struct _TestGObject TestGObject;
typedef struct _TestGObjectClass TestGObjectClass;
typedef struct _TestGObjectPrivate TestGObjectPrivate;

struct _TestGObject
{
  GObject parent;
  gint public_value;

  /*< private >*/
  TestGObjectPrivate *priv;
};

struct _TestGObjectClass
{
  GObjectClass parent_class;
};

GType test_gobject_get_type(void) G_GNUC_CONST;

TestGObject* test_gobject_new();
void test_gobject_set_public_value(TestGObject *self, gint value);
gint test_gobject_get_public_value(TestGObject *self);
void test_gobject_set_private_value(TestGObject *self, gint value);
gint test_gobject_get_private_value(TestGObject *self);

G_END_DECLS

#endif /* _TEST_SIGNALS_H_ */
