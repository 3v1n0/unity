#include "test_service_lens.h"

#include <unity.h>

G_DEFINE_TYPE(ServiceLens, service_lens, G_TYPE_OBJECT);

static void add_categories(ServiceLens* self);
static void on_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self);
static void on_global_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self);

struct _ServiceLensPrivate
{
  UnityLens* lens;
  UnityScope* scope;
};

static void
service_lens_dispose(GObject* object)
{
  ServiceLens* self = SERVICE_LENS(object);

  g_object_unref(self->priv->lens);
  g_object_unref(self->priv->scope);
}

static void
service_lens_class_init(ServiceLensClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_lens_dispose;

	g_type_class_add_private (klass, sizeof (ServiceLensPrivate));
}

static void
service_lens_init(ServiceLens* self)
{
  ServiceLensPrivate* priv;
  GError* error = NULL;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_LENS, ServiceLensPrivate);

  /* Setup Lens */
  priv->lens = unity_lens_new("/com/canonical/unity/testlens", "testlens");
  g_object_set(priv->lens,
               "search-hint", "Search Test Lens",
               "visible", TRUE,
               "search-in-global", TRUE,
               NULL);
  add_categories(self);

  /* Scope */
  priv->scope = unity_scope_new("/com/canonical/unity/testscope");
  unity_scope_set_search_in_global(priv->scope, TRUE);
  g_signal_connect(priv->scope, "notify::active-search",
                   G_CALLBACK(on_search_changed), self);
  g_signal_connect(priv->scope, "notify::active-global-search",
                   G_CALLBACK(on_global_search_changed), self);

  /* Get ready to export and export */
  unity_lens_add_local_scope(priv->lens, priv->scope);
  unity_lens_export(priv->lens, &error);
  if (error)
  {
    g_error ("Unable to export Lens: %s", error->message);
    g_error_free (error);
  }
}

static void
add_categories(ServiceLens* self)
{
  UnityGridCategory* categories[3];
    
  categories[0] = unity_grid_category_new("Category1", "gtk-apply", UNITY_GRID_CATEGORY_STYLE_ICONS);
  
  categories[1] = unity_grid_category_new("Category2", "gtk-cancel", UNITY_GRID_CATEGORY_STYLE_HORIZONTAL_ICONS);

  categories[2] = unity_grid_category_new("Category3", "gtk-close", UNITY_GRID_CATEGORY_STYLE_LIST);
 
  unity_lens_set_categories(self->priv->lens, (UnityCategory**)categories, 3);

  g_object_unref(categories[0]);
  g_object_unref(categories[1]);
  g_object_unref(categories[2]);
}

static void
on_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self)
{
  UnityLensSearch* search = unity_scope_get_active_search(self->priv->scope);
  DeeModel* model = (DeeModel*)unity_scope_get_results_model(self->priv->scope);
  int i = 0;

  for (i = 0; i < 5; i++)
  {
    gchar* name = g_strdup_printf("%s%d",
                                  unity_lens_search_get_search_string(search),
                                  i);
    dee_model_append(model,
                     "file:///test",
                     "gtk-apply",
                     i,
                     "text/html",
                     name,
                     "kamstrup likes ponies",
                     "file:///test");
    g_free(name);
  }
}

static void
on_global_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self)
{
  UnityLensSearch* search = unity_scope_get_active_global_search(self->priv->scope);
  DeeModel* model = (DeeModel*)unity_scope_get_global_results_model(self->priv->scope);
  int i = 0;

  for (i = 0; i < 10; i++)
  {
    gchar* name = g_strdup_printf("%s%d",
                                  unity_lens_search_get_search_string(search),
                                  i);
    dee_model_append(model,
                     "file:///test",
                     "gtk-apply",
                     i,
                     "text/html",
                     name,
                     "kamstrup likes ponies",
                     "file:///test");
    g_free(name);
  }
}

ServiceLens*
service_lens_new()
{
  return g_object_new(SERVICE_TYPE_LENS, NULL);
}
