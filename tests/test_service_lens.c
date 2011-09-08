#include "test_service_lens.h"

#include <unity.h>

G_DEFINE_TYPE(ServiceLens, service_lens, G_TYPE_OBJECT);

static void add_categories(ServiceLens* self);
static void add_filters(ServiceLens *self);
static void on_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self);
static void on_global_search_changed(UnityScope* scope, GParamSpec* pspec, ServiceLens* self);
static UnityActivationResponse* on_activate_uri(UnityScope* scope, const char* uri, ServiceLens* self);
static UnityPreview* on_preview_uri(UnityScope* scope, const char* uri, ServiceLens *self);

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
  add_filters(self);

  /* Scope */
  priv->scope = unity_scope_new("/com/canonical/unity/testscope");
  unity_scope_set_search_in_global(priv->scope, TRUE);

  g_signal_connect(priv->scope, "notify::active-search",
                   G_CALLBACK(on_search_changed), self);
  g_signal_connect(priv->scope, "notify::active-global-search",
                   G_CALLBACK(on_global_search_changed), self);
  g_signal_connect(priv->scope, "activate-uri",
                   G_CALLBACK(on_activate_uri), self);
  g_signal_connect(priv->scope, "preview-uri",
                   G_CALLBACK(on_preview_uri), self);

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
  GList *cats = NULL;
  GIcon *icon;

  icon = g_themed_icon_new("gtk-apply");
  cats = g_list_append (cats, unity_category_new("Category1", icon,
                                                 UNITY_CATEGORY_RENDERER_VERTICAL_TILE));
  g_object_unref (icon);
  
  icon = g_themed_icon_new("gtk-cancel");
  cats = g_list_append (cats, unity_category_new("Category2", icon,
                                                 UNITY_CATEGORY_RENDERER_HORIZONTAL_TILE));
  g_object_unref (icon);

  icon = g_themed_icon_new("gtk-close");
  cats = g_list_append (cats, unity_category_new("Category3", icon,
                                                 UNITY_CATEGORY_RENDERER_FLOW));
  g_object_unref (icon);


  unity_lens_set_categories(self->priv->lens, cats);
  g_list_free_full (cats, (GDestroyNotify) g_object_unref);
}

static void
add_filters(ServiceLens *self)
{
  GList       *filters;
  UnityFilter *filter;
  GIcon       *icon;

  filter = UNITY_FILTER (unity_radio_option_filter_new("when", "When",
                                                       NULL, FALSE));
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "today", "Today", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "yesterday", "Yesterday", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "lastweek", "Last Week", NULL);
  filters = g_list_append (filters, filter);

  filter = UNITY_FILTER (unity_check_option_filter_new("type", "Type",
                                                       NULL, FALSE));
  icon = g_themed_icon_new ("gtk-apps");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "apps", "Apps", icon);
  g_object_unref (icon);
  icon = g_themed_icon_new ("gtk-files");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "files", "Files", icon);
  g_object_unref (icon);
  //icon = g_themed_icon_new ("gtk-files");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "music", "Music", NULL);
  //g_object_unref (icon);
  filters = g_list_append (filters, filter);

  filters = g_list_append (filters, unity_ratings_filter_new("ratings",
                                                             "Ratings",
                                                             NULL, FALSE));

  filter = UNITY_FILTER (unity_multi_range_filter_new("size", "Size", NULL, TRUE));
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1MB", "1MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "10MB", "10MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "100MB", "100MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1000MB", "1000MB", NULL);
  filters = g_list_append (filters, filter);
 

  unity_lens_set_filters(self->priv->lens, filters);
  g_list_free_full (filters, (GDestroyNotify) g_object_unref);
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

static UnityActivationResponse*
on_activate_uri(UnityScope* scope, const char* uri, ServiceLens* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static UnityPreview*
on_preview_uri(UnityScope* scope, const char* uri, ServiceLens *self)
{
  gchar *genres[] = {"awesome"};
  return (UnityPreview*)unity_track_preview_new(1, "Animus Vox",
                                                "The Glitch Mob", "Drink The Sea", 
                                                404, genres, 1,
                                                "file://music/the/track", "Play",
                                                "", "play://music/the/track",
                                                "preview://music/the/track", "pause://music/the/track");
}

ServiceLens*
service_lens_new()
{
  return g_object_new(SERVICE_TYPE_LENS, NULL);
}
