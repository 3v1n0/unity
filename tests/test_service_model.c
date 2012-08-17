#include "test_service_model.h"

G_DEFINE_TYPE(ServiceModel, service_model, G_TYPE_OBJECT);

static void service_model_create_model(ServiceModel* self);
static void service_model_create_results(ServiceModel* self);
static void service_model_create_categories(ServiceModel* self);

static void
service_model_dispose(GObject* object)
{
  ServiceModel* self = SERVICE_MODEL(object);

  g_object_unref(self->model_);
  g_object_unref(self->results_model_);
  g_object_unref(self->categories_model_);
}

static void
service_model_class_init(ServiceModelClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_model_dispose;
}

static void
service_model_init(ServiceModel* self)
{
  service_model_create_model(self);
  service_model_create_results(self);
  service_model_create_categories(self);
}

static void
service_model_create_model(ServiceModel* self)
{
  self->model_ = dee_shared_model_new("com.canonical.test.model");
  dee_model_set_schema(self->model_, "u", "s", NULL);

  guint i;
  for (i = 0; i < 100; i++)
  {
    gchar* name = g_strdup_printf("Test%d", i);
    dee_model_append(self->model_, i, name);
    g_free(name);
  }
}

static void
service_model_create_results(ServiceModel* self)
{
  self->results_model_ = dee_shared_model_new("com.canonical.test.resultsmodel");
  dee_model_set_schema(self->results_model_, "s", "s", "u", "s", "s", "s", "s", NULL);

  int i;
  for(i = 0; i < 200; i++)
  {
    gchar* name = g_strdup_printf("Result%d", i);
    dee_model_append(self->results_model_,
                     name,
                     name,
                     (guint)(i/50), // new category every 50 results
                     name,
                     name,
                     name,
                     name);
    g_free(name);
  }
}

static void
service_model_create_categories(ServiceModel* self)
{
  self->categories_model_ = dee_shared_model_new("com.canonical.test.categoriesmodel");
  dee_model_set_schema(self->categories_model_, "s", "s", "s", "a{sv}", NULL);

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);

  int i;
  for(i = 0; i < 5; i++)
  {
    gchar* name = g_strdup_printf("Category%d", i);
    dee_model_append(self->categories_model_,
                     name,
                     "gtk-apply",
                     "grid",
                     hints);
    g_free(name);
  }
  g_variant_unref(hints);
}

ServiceModel*
service_model_new()
{
  return g_object_new(SERVICE_TYPE_MODEL, NULL);
}
