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

 #define _g_free_safe(var) (var = (g_free (var), NULL))

G_DEFINE_TYPE(ServiceScope, service_scope, G_TYPE_OBJECT);

static void add_categories(ServiceScope* self);
static void add_filters(ServiceScope *self);
static UnityActivationResponse* on_activate_uri(TestScope* scope, const char* uri, ServiceScope* self);
static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_ctx, gpointer self);

static GObject * service_scope_constructor (GType gtype, guint n_properties, GObjectConstructParam *properties);
static void service_scope_set_scope_id (ServiceScope* self, const gchar* value);
static const gchar* service_scope_get_scope_id (ServiceScope* self);
static void service_scope_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void service_scope_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);

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

struct _ServiceScopePrivate
{
  TestScope* scope;
  gchar* _scope_id;
};

enum  {
  SERVICE_SCOPE_DUMMY_PROPERTY,
  SERVICE_SCOPE_SCOPE_ID
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

  G_OBJECT_CLASS (klass)->constructor = service_scope_constructor;
  G_OBJECT_CLASS (klass)->get_property = service_scope_get_property;
  G_OBJECT_CLASS (klass)->set_property = service_scope_set_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), SERVICE_SCOPE_SCOPE_ID, g_param_spec_string ("scope-id", "scope-id", "scope-id", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

static void
service_scope_init(ServiceScope* self)
{
  ServiceScopePrivate* priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_SCOPE, ServiceScopePrivate);
  priv->scope = NULL;
  priv->_scope_id = NULL;
}

static GObject *
service_scope_constructor(GType                  gtype,
                          guint                  n_properties,
                          GObjectConstructParam *properties)
{
  GObject *obj;
  {
    obj = G_OBJECT_CLASS (service_scope_parent_class)->constructor (gtype, n_properties, properties);
  }

  GError* error = NULL;
  ServiceScope * self;
  self = SERVICE_SCOPE (obj);
  if (!self)
    return;

  gchar* scope_name = g_strdup_printf("/com/canonical/unity/scope/%s", self->priv->_scope_id);
  /* Scope */
  self->priv->scope = test_scope_new(scope_name);
  add_categories(self);
  add_filters(self);

  g_signal_connect(self->priv->scope, "activate-uri",
                   G_CALLBACK(on_activate_uri), self);

  g_signal_connect(self->priv->scope, "search",
                   G_CALLBACK(on_scope_search), self);

  /* Export */
  test_scope_export(self->priv->scope, &error);
  if (error)
  {
    g_error ("Unable to export Scope: %s", error->message);
    g_error_free (error);
  }

  g_free(scope_name);

  return obj;
}

static void service_scope_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  ServiceScope * self;
  self = SERVICE_SCOPE (object);
  switch (property_id)
  {
    case SERVICE_SCOPE_SCOPE_ID:
    g_value_set_string (value, service_scope_get_scope_id (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void service_scope_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  ServiceScope * self;
  self = SERVICE_SCOPE (object);
  switch (property_id) {
    case SERVICE_SCOPE_SCOPE_ID:
    service_scope_set_scope_id (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static const gchar* service_scope_get_scope_id (ServiceScope* self)
{
  g_return_val_if_fail (self != NULL, NULL);
  return self->priv->_scope_id;
}

static void service_scope_set_scope_id (ServiceScope* self, const gchar* value)
{
  g_return_if_fail (self != NULL);
  _g_free_safe (self->priv->_scope_id);
  self->priv->_scope_id = g_strdup (value);;
  g_object_notify ((GObject *) self, "scope-id");
}


static void
add_categories(ServiceScope* self)
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

static void on_scope_search (TestScope* scope, UnityScopeSearchBase* search_base, gpointer self)
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
    result.icon_hint    = g_strdup(icons[i % 10]);
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

ServiceScope* service_scope_new(const gchar* scope_name)
{
  return g_object_new(SERVICE_TYPE_SCOPE, "scope-id", scope_name, NULL);
}

