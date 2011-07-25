#include "test_service_lens.h"

#include <unity.h>

G_DEFINE_TYPE(ServiceLens, service_lens, G_TYPE_OBJECT);

static void add_categories(ServiceLens* self);

struct _ServiceLensPrivate
{
  UnityLens* lens;
};

static void
service_lens_dispose(GObject* object)
{
  //ServiceLens* self = SERVICE_LENS(object);
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

  priv->lens = unity_lens_new("/com/canonical/unity/testlens", "testlens");
  g_object_set(priv->lens,
               "search-hint", "Search Test Lens",
               "visible", TRUE,
               "search-in-global", TRUE,
               NULL);
  add_categories(self);

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

ServiceLens*
service_lens_new()
{
  return g_object_new(SERVICE_TYPE_LENS, NULL);
}
