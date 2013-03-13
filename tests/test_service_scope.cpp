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

#include "stdio.h"
#include "test_service_scope.h"

namespace unity
{
namespace service
{

#define _g_free_safe(var) (var = (g_free (var), NULL))

const gchar* icons[] = {  "gtk-cdrom",
                          "gtk-directory",
                          "gtk-clear",
                          "gtk-find",
                          "gtk-bold",
                          "gtk-copy",
                          "gtk-cut",
                          "gtk-delete",
                          "gtk-dialog-authentication",
                          "gtk-dialog-error",
                          "gtk-dialog-info" };

const gchar* category_ids[] = { "cat0",
                                "cat1",
                                "cat2" };

static UnityActivationResponse* on_activate_uri(TestScope* scope, const char* uri, Scope* self);
static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_ctx, Scope* self);

Scope::Scope(std::string const& scope_id)
: scope_(test_scope_new (("/com/canonical/unity/scope/"+scope_id).c_str()))
{
  AddCategories();
  AddFilters();

  g_signal_connect(scope_, "activate-uri", G_CALLBACK(on_activate_uri), this);
  g_signal_connect(scope_, "search", G_CALLBACK(on_scope_search), this);

  /* Export */
  GError* error = NULL;
  test_scope_export(scope_, &error);
  if (error)
  {
    g_error ("Unable to export Scope: %s", error->message);
    g_error_free (error);
  }
}

Scope::~Scope()
{
  g_signal_handlers_disconnect_by_data(scope_, this);
}

void Scope::AddCategories()
{
  UnityCategorySet* categories;
  GIcon *icon;
  UnityCategory* cateogry;

  categories = unity_category_set_new();

  int i = 0;
  int sizeof_categories = sizeof(category_ids) / sizeof(gchar*);
  int sizeof_icons = sizeof(icons) / sizeof(gchar*);

  for (i = 0; i < sizeof_categories; i++)
  {
    gchar* title = g_strdup_printf("Category %d", i);

    icon = g_themed_icon_new(icons[i % sizeof_icons]);
    cateogry = unity_category_new(category_ids[i], title, icon,
                                                   UNITY_CATEGORY_RENDERER_VERTICAL_TILE);
    unity_category_set_add(categories, cateogry);
    g_object_unref (cateogry);
    g_object_unref (icon);

    g_free(title);
  }

  test_scope_set_categories(scope_, categories);
  g_object_unref (categories);
}

void Scope::AddFilters()
{
  UnityFilterSet *filters = NULL;
  UnityFilter *filter;
  GIcon       *icon;

  filters = unity_filter_set_new();

  // Check option filter - Categories
  filter = UNITY_FILTER (unity_check_option_filter_new("categories", "Categories",
                                                       NULL, FALSE));

  int i = 0;
  int sizeof_categories = sizeof(category_ids) / sizeof(gchar*);
  int sizeof_icons = sizeof(icons) / sizeof(gchar*);

  for (i = 0; i < sizeof_categories; i++)
  {
    gchar* title = g_strdup_printf("Category %d", i);

    icon = g_themed_icon_new(icons[i % sizeof_icons]);
    unity_options_filter_add_option(UNITY_OPTIONS_FILTER (filter),
                                    category_ids[i], title, icon);
    g_object_unref (icon);
    g_free(title);
  }

  unity_filter_set_add (filters, filter);
  g_object_unref(filter);

  // Radio optoin filter
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

  // Rating filter
  filter = UNITY_FILTER (unity_ratings_filter_new("ratings",
                                    "Ratings",
                                    NULL,
                                    FALSE));
  unity_filter_set_add (filters, filter);
  g_object_unref(filter);

  // Range filter
  filter = UNITY_FILTER (unity_multi_range_filter_new("size", "Size", NULL, TRUE));
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1MB", "1MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "10MB", "10MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "100MB", "100MB", NULL);
  unity_options_filter_add_option(UNITY_OPTIONS_FILTER(filter), "1000MB", "1000MB", NULL);
  unity_filter_set_add (filters, filter);
  g_object_unref(filter); 

  test_scope_set_filters(scope_, filters);
  g_object_unref (filters);
}

static UnityActivationResponse* on_activate_uri(TestScope* scope, const char* uri, Scope* self)
{
  return unity_activation_response_new(UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static void _g_variant_unref0_ (gpointer var) {
  if (var) g_variant_unref ((GVariant*)var);
}

static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_base, Scope* self)
{
  UnitySearchContext* search_ctx;
  search_ctx = search_base->search_context;
  g_return_if_fail (search_ctx != NULL);

  int num_items = 0;
  gchar* search_title = g_strdup(search_ctx->search_query);
  gchar* search_title_tmp = g_strdup(search_title);

  // cheeky search string format to control how many results to return
  // count:title
  if (g_strcmp0(search_title, "") == 0)
  {
    num_items = 64;
  }
  else
  {
    int sscanf_n = sscanf(search_title, "%d:%s", &num_items, search_title_tmp);
    switch (sscanf_n)
    {
      case 0:
        g_free(search_title_tmp);
        num_items = 64;
        break;

      case 1:
        g_free(search_title);
        g_free(search_title_tmp);
        search_title = g_strdup("");
        break;

      case 2:
      default:
        g_free(search_title);
        search_title = search_title_tmp;
        break;
    }
  }

  UnityScopeResult result;
  int i;

  int sizeof_categories = sizeof(category_ids) / sizeof(gchar*);
  int sizeof_icons = sizeof(icons) / sizeof(gchar*);

  UnityOptionsFilter* options_filter = UNITY_OPTIONS_FILTER (unity_filter_set_get_filter_by_id(search_ctx->filter_state, "categories"));

  for (i = 0; i < num_items; i++)
  {
    memset (&result, 0, sizeof (UnityScopeResult));

    int category = i % sizeof_categories;
    const gchar* category_id = category_ids[category];

    if (options_filter && unity_filter_get_filtering(UNITY_FILTER (options_filter)))
    {
      UnityFilterOption* filter_option = unity_options_filter_get_option(options_filter, category_id);

      if (filter_option && !unity_filter_option_get_active(filter_option))
        continue;
    }

    // Match title to search string.
    gchar* title = g_strdup_printf("%s.%d", category_id, i);
    if (search_title &&
        g_strcmp0(search_title, "") != 0 &&
        g_strrstr(title, search_title) == NULL)
    {
      g_free(title);
      continue;
    }

    result.uri          = g_strdup_printf("test://uri.%d", i);
    result.title        = title;
    result.icon_hint    = g_strdup(icons[i % sizeof_icons]);
    result.result_type  = UNITY_RESULT_TYPE_DEFAULT;
    result.category     = (guint) (category),         // 3 categoies
    result.mimetype     = g_strdup("inode/folder");
    result.comment      = g_strdup_printf("Comment %d", i);
    result.dnd_uri      = g_strdup_printf("test://dnd_uri.%d", i);

    result.metadata = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, _g_variant_unref0_);

    g_hash_table_insert (result.metadata, g_strdup ("required_int"), g_variant_ref_sink (g_variant_new_int32 (5)));
    g_hash_table_insert (result.metadata, g_strdup ("required_string"), g_variant_ref_sink (g_variant_new_string ("foo")));

    unity_result_set_add_result (search_ctx->result_set, &result);    
    
    unity_scope_result_destroy (&result);
  }
  g_free(search_title);
}

}
}
