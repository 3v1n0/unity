#include "test_glib_object_utils.h"

struct _TestGObjectPrivate
{
  int private_value;
};

G_DEFINE_TYPE(TestGObject, test_gobject, G_TYPE_OBJECT);

static void
test_gobject_class_init(TestGObjectClass* klass)
{
  g_type_class_add_private (klass, sizeof (TestGObjectPrivate));
}

static void
test_gobject_init(TestGObject* self)
{
  TestGObjectPrivate *priv;
  self->priv = TEST_GOBJECT_GET_PRIVATE(self);
  priv = self->priv;

  priv->private_value = 55;
}

TestGObject*
test_gobject_new()
{
  return TEST_GOBJECT(g_object_new(TEST_TYPE_GOBJECT, NULL));
}

void test_gobject_set_private_value(TestGObject *self, gint value)
{
  TestGObjectPrivate *priv;
  g_return_if_fail(TEST_IS_GOBJECT(self));

  priv = TEST_GOBJECT_GET_PRIVATE(self);
  priv->private_value = value;
}

gint test_gobject_get_private_value(TestGObject *self)
{
  TestGObjectPrivate *priv;
  g_return_val_if_fail(TEST_IS_GOBJECT(self), 0);

  priv = TEST_GOBJECT_GET_PRIVATE(self);
  return priv->private_value;
}

void test_gobject_set_public_value(TestGObject *self, gint value)
{
  g_return_if_fail(TEST_IS_GOBJECT(self));

  self->public_value = value;
}

gint test_gobject_get_public_value(TestGObject *self)
{
  g_return_val_if_fail(TEST_IS_GOBJECT(self), 0);

  return self->public_value;
}
