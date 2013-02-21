#include "test_service_model.h"
#include <dee.h>

G_DEFINE_TYPE(ServiceModel, service_model, G_TYPE_OBJECT);

struct _ServiceModelPrivate
{
  DeeModel* model_;
  DeeModel* results_model_;
  DeeModel* categories_model_;

  DeeModel* categories_model_changing_;
  guint timout_id_;
};


static void service_model_create_model(ServiceModel* self);
static void service_model_create_results(ServiceModel* self);
static void service_model_create_categories(ServiceModel* self);
static void service_model_create_categories_changing(ServiceModel* self);
static gboolean service_model_categories_add_remove_change(gpointer user_data);

static void
service_model_dispose(GObject* object)
{
  ServiceModel* self = SERVICE_MODEL(object);

  g_object_unref(self->priv->model_);
  g_object_unref(self->priv->results_model_);
  g_object_unref(self->priv->categories_model_);
  g_object_unref(self->priv->categories_model_changing_);
  g_source_remove(self->priv->timout_id_);

  G_OBJECT_CLASS (service_model_parent_class)->dispose (object);
}

static void
service_model_class_init(ServiceModelClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_model_dispose;

  g_type_class_add_private (klass, sizeof (ServiceModelPrivate));
}

static void
service_model_init(ServiceModel* self)
{
  ServiceModelPrivate* priv;
  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_MODEL, ServiceModelPrivate);

  service_model_create_model(self);
  service_model_create_results(self);
  service_model_create_categories(self);
  service_model_create_categories_changing(self);
}

static void
service_model_create_model(ServiceModel* self)
{
  self->priv->model_ = dee_shared_model_new("com.canonical.test.model");
  dee_model_set_schema(self->priv->model_, "u", "s", NULL);

  guint i;
  for (i = 0; i < 100; i++)
  {
    gchar* name = g_strdup_printf("Test%d", i);
    dee_model_append(self->priv->model_, i, name);
    g_free(name);
  }
}

static void
service_model_create_results(ServiceModel* self)
{
  self->priv->results_model_ = dee_shared_model_new("com.canonical.test.resultsmodel");
  dee_model_set_schema(self->priv->results_model_, "s", "s", "u", "u", "s", "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "key", g_variant_new_string("value"));
  GVariant *hints = g_variant_builder_end(&b);
  
  int i;
  for(i = 0; i < 200; i++)
  {
    gchar* name = g_strdup_printf("Result%d", i);
    dee_model_append(self->priv->results_model_,
                     name,
                     name,
                     (guint)(i/50), // new category every 50 results
                     0,
                     name,
                     name,
                     name,
                     name,
                     hints);
    g_free(name);
  }
  g_variant_unref(hints);
}

static void
service_model_create_categories(ServiceModel* self)
{
  self->priv->categories_model_ = dee_shared_model_new("com.canonical.test.categoriesmodel");
  dee_model_set_schema(self->priv->categories_model_, "s", "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);
  g_variant_ref_sink(hints);

  int i;
  for(i = 0; i < 5; i++)
  {
    gchar* id = g_strdup_printf("cat%d", i);
    gchar* name = g_strdup_printf("Category %d", i);
    dee_model_append(self->priv->categories_model_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
  }
  g_variant_unref(hints);
}


static gboolean
service_model_categories_add_remove_change(gpointer user_data)
{
  ServiceModel* self = SERVICE_MODEL(user_data);
  if (!self)
    return TRUE;

  DeeModelIter* iter = dee_model_get_first_iter(self->priv->categories_model_changing_);
  if (!iter)
    return TRUE;

  static int new_category = 0;

  // remove first row.
  dee_model_remove(self->priv->categories_model_changing_, iter);

  // change first
  iter = dee_model_get_first_iter(self->priv->categories_model_changing_);
  if (iter)
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    GVariant *hints = g_variant_builder_end(&b);
    g_variant_ref_sink(hints);

    gchar* id = g_strdup_printf("cat%d", new_category);
    gchar* name = g_strdup_printf("Category %d", new_category);
    dee_model_set(self->priv->categories_model_changing_,
                  iter,
                  id,
                  name,
                  "gtk-apply",
                  "grid",
                  hints);
    g_free(id);
    g_free(name);
    g_variant_unref(hints);

    new_category++;
  }

  // append new,
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    GVariant *hints = g_variant_builder_end(&b);
    g_variant_ref_sink(hints);

    gchar* id = g_strdup_printf("cat%d", new_category);
    gchar* name = g_strdup_printf("Category %d", new_category);
    dee_model_append(self->priv->categories_model_changing_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
    g_variant_unref(hints);

    new_category++;
  }

  return TRUE;
}

static void
service_model_create_categories_changing(ServiceModel* self)
{
  self->priv->categories_model_changing_ = dee_shared_model_new("com.canonical.test.categoriesmodel_changing");
  dee_model_set_schema(self->priv->categories_model_changing_, "s", "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);
  g_variant_ref_sink(hints);

  int i;
  for(i = 0; i < 5; i++)
  {
    gchar* id = g_strdup_printf("cat%d", i);
    gchar* name = g_strdup_printf("Category %d", i);
    dee_model_append(self->priv->categories_model_changing_,
                     id,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(id);
    g_free(name);
  }
  g_variant_unref(hints);


  self->priv->timout_id_ = g_timeout_add(100, service_model_categories_add_remove_change, self);
}

ServiceModel*
service_model_new()
{
  return g_object_new(SERVICE_TYPE_MODEL, NULL);
}
