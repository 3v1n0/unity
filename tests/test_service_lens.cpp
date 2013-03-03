#include "test_service_lens.h"

#include <unity.h>

namespace unity
{
namespace service
{
static void on_search_changed(UnityScope* scope, UnityLensSearch *lens_search, UnitySearchType search_type, GCancellable *canc, Lens* self);
static UnityActivationResponse* on_activate_uri(UnityScope* scope, const char* uri, Lens* self);
static UnityPreview* on_preview_uri(UnityScope* scope, const char* uri, Lens *self);

Lens::Lens()
  : lens_(unity_lens_new("/com/canonical/unity/testlens", "testlens"))
  , scope_(unity_scope_new("/com/canonical/unity/testscope"))
{
  g_object_set(lens_, "search-hint", "Search Test Lens", "visible", TRUE,
               "search-in-global", TRUE, NULL);

  AddCategories();
  AddFilters();

  unity_scope_set_search_in_global(scope_, TRUE);

  // sig_manager_.Add<UnityScope*, UnityLensSearch*, UnitySearchType, GCancellable*>(scope_, "search-changed")
  // void Add(G object, std::string const& signal_name, typename Signal<R, G, Ts...>::SignalCallback const& callback)

  g_signal_connect(scope_, "search-changed", G_CALLBACK(on_search_changed), this);
  g_signal_connect(scope_, "activate-uri", G_CALLBACK(on_activate_uri), this);
  g_signal_connect(scope_, "preview-uri", G_CALLBACK(on_preview_uri), this);

  /* Get ready to export and export */
  glib::Error error;
  unity_lens_add_local_scope(lens_, scope_);
  unity_lens_export(lens_, &error);

  if (error)
  {
    g_error ("Unable to export Lens: %s", error.Message().c_str());
  }
}

Lens::~Lens()
{
  g_signal_handlers_disconnect_by_data(scope_, this);
}

void Lens::AddCategories()
{
  GList *cats = NULL;
  glib::Object<GIcon> icon;

  icon = g_themed_icon_new("gtk-apply");
  cats = g_list_append (cats, unity_category_new("Category1", icon,
                                                 UNITY_CATEGORY_RENDERER_VERTICAL_TILE));

  icon = g_themed_icon_new("gtk-cancel");
  cats = g_list_append (cats, unity_category_new("Category2", icon,
                                                 UNITY_CATEGORY_RENDERER_HORIZONTAL_TILE));

  icon = g_themed_icon_new("gtk-close");
  cats = g_list_append (cats, unity_category_new("Category3", icon,
                                                 UNITY_CATEGORY_RENDERER_FLOW));


  unity_lens_set_categories(lens_, cats);
  g_list_free_full (cats, (GDestroyNotify) g_object_unref);
}

void Lens::AddFilters()
{
  GList       *filters = NULL;
  UnityFilter *filter;
  glib::Object<GIcon> icon;

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

  icon = g_themed_icon_new ("gtk-files");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "files", "Files", icon);

  icon = g_themed_icon_new ("gtk-music");
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "music", "Music", icon);

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

  unity_lens_set_filters(lens_, filters);
  g_list_free_full (filters, (GDestroyNotify) g_object_unref);
}


static void on_search_changed(UnityScope* scope, UnityLensSearch *search, UnitySearchType search_type, GCancellable *canc, Lens* self)
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

static UnityActivationResponse* on_activate_uri(UnityScope* scope, const char* uri, Lens* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static UnityActivationResponse* preview_action_activated(UnityPreviewAction* action, const char* uri)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_SHOW_DASH, "");
}

static UnityPreview* on_preview_uri(UnityScope* scope, const char* uri, Lens *self)
{
  UnityPreviewAction* action;
  UnityMoviePreview* preview;

  preview = unity_movie_preview_new("A movie", "With subtitle",
                                    "And description", NULL);

  action = unity_preview_action_new("action_A", "An action", NULL);
  unity_preview_add_action(UNITY_PREVIEW(preview), action);
  g_signal_connect(action, "activated",
                   G_CALLBACK(preview_action_activated), NULL);

  return (UnityPreview*) preview;
}

}
}
