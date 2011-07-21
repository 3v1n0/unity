#include <glib-object.h>
#include "test_service_model.h"

gint
main(gint argc, gchar** argv)
{
  GMainLoop *loop;
  ServiceModel* service_model;

  g_type_init();

  loop = g_main_loop_new(NULL, FALSE);

  /* The services */
  service_model = service_model_new();

  g_main_loop_run(loop);
  
  g_object_unref(service_model);
  g_main_loop_unref(loop);

  return 0;
}
