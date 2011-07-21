#include "test_service_model.h"

G_DEFINE_TYPE(ServiceModel, service_model, G_TYPE_OBJECT);

static void
service_model_class_init(ServiceModelClass* klass)
{}

static void
service_model_init(ServiceModel* self)
{
  self->model_ = dee_shared_model_new("com.canonical.test.model");
  dee_model_set_schema(self->model_, "i", "s", NULL);

  int i;
  for (i = 0; i < 100; i++)
  {
    gchar* name = g_strdup_printf("Test%d", i);
    dee_model_append(self->model_, i, name);
    g_free(name);
  }

}

ServiceModel*
service_model_new()
{
  return g_object_new(SERVICE_TYPE_MODEL, NULL);
}
