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
#include "test_scope_impl.h"

G_DEFINE_TYPE(ServiceScope, service_scope, G_TYPE_OBJECT);

static void add_categories(ServiceScope* self);
static void add_filters(ServiceScope *self);
static UnityActivationResponse* on_activate_uri(TestScope* scope, const char* uri, ServiceScope* self);
static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_ctx, gpointer self);


struct _ServiceScopePrivate
{
  TestScope* scope;
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
  priv->scope = test_scope_new("/com/canonical/unity/scope/testscope1");
  add_categories(self);
  add_filters(self);

  g_signal_connect(priv->scope, "activate-uri",
                   G_CALLBACK(on_activate_uri), self);

  g_signal_connect(priv->scope, "search",
                   G_CALLBACK(on_scope_search), self);

  /* Export */
  test_scope_export(priv->scope, &error);
  if (error)
  {
    g_error ("Unable to export Scope: %s", error->message);
    g_error_free (error);
  }
}

static void
add_categories(ServiceScope* self)
{
  UnityCategorySet* categories;
  GIcon *icon;
  UnityCategory* cateogry;

  categories = unity_category_set_new();

  icon = g_themed_icon_new("gtk-apply");
  cateogry = unity_category_new("cat1", "Cateogry 1", icon,
                                                 UNITY_CATEGORY_RENDERER_VERTICAL_TILE);
  unity_category_set_add(categories, cateogry);
  g_object_unref (cateogry);
  g_object_unref (icon);
  
  icon = g_themed_icon_new("gtk-cancel");
  cateogry = unity_category_new("cat2", "Category 2", icon,
                                                 UNITY_CATEGORY_RENDERER_HORIZONTAL_TILE);
  unity_category_set_add(categories, cateogry);
  g_object_unref (cateogry);
  g_object_unref (icon);

  icon = g_themed_icon_new("gtk-close");
  cateogry = unity_category_new("cat3", "Category 3", icon,
                                                 UNITY_CATEGORY_RENDERER_FLOW);
  unity_category_set_add(categories, cateogry);
  g_object_unref (cateogry);
  g_object_unref (icon);

  test_scope_set_categories(self->priv->scope, categories);
  g_object_unref (categories);
}

static void
add_filters(ServiceScope *self)
{
  UnityFilterSet *filters = NULL;
  UnityFilter *filter;
  GIcon       *icon;

  filters = unity_filter_set_new();

  filter = UNITY_FILTER (unity_radio_option_filter_new("when", "When",
                                                       NULL, FALSE));
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "today", "Today", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "yesterday", "Yesterday", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                  "lastweek", "Last Week", NULL);
  unity_filter_set_add (filters, filter);
  g_object_unref(filter);

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
  unity_filter_set_add (filters, filter);
  g_object_unref(filter);


  filter = UNITY_FILTER (unity_ratings_filter_new("ratings",
                                    "Ratings",
                                    NULL,
                                    FALSE));
  unity_filter_set_add (filters, filter);
  g_object_unref(filter);

  filter = UNITY_FILTER (unity_multi_range_filter_new("size", "Size", NULL, TRUE));
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1MB", "1MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "10MB", "10MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "100MB", "100MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1000MB", "1000MB", NULL);
  unity_filter_set_add (filters, filter);
  g_object_unref(filter); 

  test_scope_set_filters(self->priv->scope, filters);
  g_object_unref (filters);
}

static UnityActivationResponse* on_activate_uri(TestScope* scope, const char* uri, ServiceScope* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static void _g_variant_unref0_ (gpointer var) {
  (var == NULL) ? NULL : (var = (g_variant_unref (var), NULL));
}

static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_ctx, gpointer self)
{
  UnityScopeResult result;

  int i;
  for (i = 0; i < 10; i++)
  {
    memset (&result, 0, sizeof (UnityScopeResult));

    result.uri          = g_strdup_printf("test://uri.%d", i);
    result.icon_hint    = g_strdup("");
    result.result_type  = UNITY_RESULT_TYPE_DEFAULT;
    result.category     = (guint) 0;
    result.title        = g_strdup_printf("Title %d", i);
    result.mimetype     = g_strdup("inode/folder");
    result.comment      = g_strdup_printf("Comment %d", i);
    result.dnd_uri      = g_strdup_printf("test://dnd_uri.%d", i);

    result.metadata = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, _g_variant_unref0_);

    g_hash_table_insert (result.metadata, g_strdup ("required_int"), g_variant_ref_sink (g_variant_new_int32 (5)));
    g_hash_table_insert (result.metadata, g_strdup ("required_string"), g_variant_ref_sink (g_variant_new_string ("foo")));

    unity_result_set_add_result (search_ctx->search_context->result_set, &result);    
  }

  unity_scope_result_destroy (&result);
}

ServiceScope* service_scope_new()
{
  return g_object_new(SERVICE_TYPE_SCOPE, NULL);
}

