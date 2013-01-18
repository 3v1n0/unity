// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

 #include "test_service_scope.h"

#include <unity.h>
 #include "stdio.h"

G_DEFINE_TYPE(ServiceScope, service_scope, G_TYPE_OBJECT);

static void add_categories(ServiceScope* self);
static void add_filters(ServiceScope *self);
static void on_search_changed(UnityScope* scope, UnityScopeSearch *search, UnitySearchType search_type, GCancellable *canc, ServiceScope* self);
static UnityActivationResponse* on_activate_uri(UnityScope* scope, const char* uri, ServiceScope* self);
static UnityPreview* on_preview_uri(UnityScope* scope, const char* uri, ServiceScope *self);

struct _ServiceScopePrivate
{
  UnityAbstractScope* scope;
};

static void
service_scope_dispose(GObject* object)
{
  ServiceScope* self = SERVICE_SCOPE(object);

  g_object_unref(self->priv->scope);
}

static void
service_scope_class_init(ServiceScopeClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_scope_dispose;

	g_type_class_add_private (klass, sizeof (ServiceScopePrivate));
}

static void
service_scope_init(ServiceScope* self)
{
  ServiceScopePrivate* priv;
  GError* error = NULL;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_SCOPE, ServiceScopePrivate);

  /* Scope */
  priv->scope = UNITY_ABSTRACT_SCOPE(unity_scope_new("/com/canonical/unity/scope/testscope1", "TestScope1"));
  unity_abstract_scope_set_search_in_global(priv->scope, TRUE);
  unity_abstract_scope_set_visible(priv->scope, TRUE);
  unity_abstract_scope_set_search_hint(priv->scope, "Search Test Scope");

  add_categories(self);
  add_filters(self);

  g_signal_connect(priv->scope, "search-changed",
                   G_CALLBACK(on_search_changed), self);
  g_signal_connect(priv->scope, "activate-uri",
                   G_CALLBACK(on_activate_uri), self);
  g_signal_connect(priv->scope, "preview-uri",
                   G_CALLBACK(on_preview_uri), self);

  /* Export */
  unity_abstract_scope_export(priv->scope, &error);
  if (error)
  {
    g_error ("Unable to export Scope: %s", error->message);
    g_error_free (error);
  }
}

static void
add_categories(ServiceScope* self)
{
  GList *cats = NULL;
  GIcon *icon;

  icon = g_themed_icon_new("gtk-apply");
  cats = g_list_append (cats, unity_category_new("cat1", "Cateogry 1", icon,
                                                 UNITY_CATEGORY_RENDERER_VERTICAL_TILE));
  g_object_unref (icon);
  
  icon = g_themed_icon_new("gtk-cancel");
  cats = g_list_append (cats, unity_category_new("cat2", "Category 2", icon,
                                                 UNITY_CATEGORY_RENDERER_HORIZONTAL_TILE));
  g_object_unref (icon);

  icon = g_themed_icon_new("gtk-close");
  cats = g_list_append (cats, unity_category_new("cat3", "Category 3", icon,
                                                 UNITY_CATEGORY_RENDERER_FLOW));
  g_object_unref (icon);


  unity_abstract_scope_set_categories(self->priv->scope, cats);
  g_list_free_full (cats, (GDestroyNotify) g_object_unref);
}

static void
add_filters(ServiceScope *self)
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
 

  unity_abstract_scope_set_filters(self->priv->scope, filters);
  g_list_free_full (filters, (GDestroyNotify) g_object_unref);
}

static void
on_search_changed(UnityScope* scope, UnityScopeSearch *search,
    UnitySearchType search_type, GCancellable *canc, ServiceScope* self)
{
  int i = 0;
  UnityScopeSearchBase* base_search = UNITY_SCOPE_SEARCH_BASE(search);
  if (!base_search)
    return;

  // cheeky search string format to control how many results to return
  // count:title

  int num_items = 0;
  const gchar* search_string = unity_scope_search_base_get_search_string(base_search);
  gchar* search_title = g_strnfill(strlen(search_string), 0);

  if (sscanf(search_string, "%d:%s", &num_items, search_title) < 1)
    num_items = 6;
  if (g_strcmp0(search_title, "") == 0)
  {
    g_free(search_title);
    search_title = g_strdup("global_result");
  }


  DeeModel* model = (DeeModel*)unity_scope_search_base_get_results_model(base_search);

  for (i = 0; i < num_items; i++)
  {
    gchar* name = g_strdup_printf("%s%d",
                                  search_title,
                                  i);

    dee_model_append(model,
                     "file:///test",
                     "gtk-apply",
                     i%3,         // 3 categoies, 4 results in each
                     0,
                     "text/html",
                     name,
                     "kamstrup likes ponies",
                     "file:///test",
                     g_variant_new_array (G_VARIANT_TYPE("{sv}"), NULL, 0));
    g_free(name);
  }
  g_free(search_title);

  unity_scope_search_finished (search);
}

static UnityActivationResponse*
on_activate_uri(UnityScope* scope, const char* uri, ServiceScope* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static UnityActivationResponse*
preview_action_activated(UnityPreviewAction* action, const char* uri)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_SHOW_DASH, "");
}

static UnityPreview*
on_preview_uri(UnityScope* scope, const char* uri, ServiceScope *self)
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

ServiceScope*
service_scope_new()
{
  return g_object_new(SERVICE_TYPE_SCOPE, NULL);
}
