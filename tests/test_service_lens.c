#include "test_service_lens.h"

#include <unity.h>

G_DEFINE_TYPE(ServiceLens, service_lens, G_TYPE_OBJECT);

static void add_categories(ServiceLens* self);
static void add_filters(ServiceLens *self);
static void on_search_changed(UnityScope* scope, UnityLensSearch *lens_search, UnitySearchType search_type, GCancellable *canc, ServiceLens* self);
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

  g_signal_connect(priv->scope, "search-changed",
                   G_CALLBACK(on_search_changed), self);
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
  GList       *filters = NULL;
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
  icon = g_themed_icon_new ("gtk-music");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "music", "Music", icon);
  g_object_unref (icon);
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
on_search_changed(UnityScope* scope, UnityLensSearch *search,
    UnitySearchType search_type, GCancellable *canc, ServiceLens* self)
{
  int i = 0;
  // to differentiate global and non-global searches, we'll return more items
  // in the case of global search
  int num_items = search_type == UNITY_SEARCH_TYPE_GLOBAL ? 10 : 5;

  DeeModel* model = (DeeModel*)unity_lens_search_get_results_model(search);

  for (i = 0; i < num_items; i++)
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

  unity_lens_search_finished (search);
}

static UnityActivationResponse*
on_activate_uri(UnityScope* scope, const char* uri, ServiceLens* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static UnityActivationResponse*
preview_action_activated(UnityPreviewAction* action, const char* uri)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_SHOW_DASH, "");
}

static UnityPreview*
generate_child_preview(UnitySeriesPreview* parent, const char* uri)
{
  UnityPreview* preview;
  UnityPreviewAction *action;

  gchar* desc = g_strdup_printf("Description for an item with uri %s", uri);
  preview = (UnityPreview*) unity_generic_preview_new("A preview", desc, NULL);
  action = unity_preview_action_new("child_action_X", "An action", NULL);
  unity_preview_add_action(preview, action);
  g_signal_connect(action, "activated",
                   G_CALLBACK(preview_action_activated), NULL);

  g_free(desc);
  return preview;
}

static UnityPreview*
on_preview_uri(UnityScope* scope, const char* uri, ServiceLens *self)
{
  UnityPreviewAction* action;
  UnitySeriesPreview* preview;
  UnitySeriesItem* series_items[4];

  series_items[0] = unity_series_item_new("scheme://item/1", "Item #1", NULL);
  series_items[1] = unity_series_item_new("scheme://item/2", "Item #2", NULL);
  series_items[2] = unity_series_item_new("scheme://item/3", "Item #3", NULL);
  series_items[3] = unity_series_item_new("scheme://item/4", "Item #4", NULL);

  preview = unity_series_preview_new(series_items, 4, "scheme://item/3");
  g_signal_connect(preview, "request-item-preview",
                   G_CALLBACK(generate_child_preview), NULL);

  action = unity_preview_action_new("series_action_A", "An action", NULL);
  unity_preview_add_action(UNITY_PREVIEW(preview), action);
  g_signal_connect(action, "activated",
                   G_CALLBACK(preview_action_activated), NULL);

  return (UnityPreview*) preview;
}

ServiceLens*
service_lens_new()
{
  return g_object_new(SERVICE_TYPE_LENS, NULL);
}
