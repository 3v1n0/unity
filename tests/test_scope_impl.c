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
 
#include "test_scope_impl.h"
#include <unity.h>

#include <stdio.h>

#define TEST_DBUS_NAME "com.canonical.Unity.Test.Scope"

static UnityAbstractPreview* test_scope_preview(UnityResultPreviewer* self, gpointer user_data)
{
  UnityAbstractPreview* preview;
  UnityPreviewAction* action;
  preview = UNITY_ABSTRACT_PREVIEW (unity_generic_preview_new ("title", "description", NULL));

  action = unity_preview_action_new ("action1", "Action 1", NULL);
  unity_preview_add_action(UNITY_PREVIEW (preview), action);

  action = unity_preview_action_new ("action2", "Action 2", NULL);
  unity_preview_add_action(UNITY_PREVIEW (preview), action);

  return preview;
}

static UnityActivationResponse* test_scope_activate(UnityScopeResult* result, UnitySearchMetadata* metadata, const gchar* action_id, gpointer data)
{
  return unity_activation_response_new (UNITY_HANDLED_TYPE_HIDE_DASH, "");
}

static UnitySchema* test_scope_get_schema(void)
{
  UnitySchema* schema = unity_schema_new ();
  unity_schema_add_field (schema, "required_string", "s", UNITY_SCHEMA_FIELD_TYPE_REQUIRED);
  unity_schema_add_field (schema, "required_int", "i", UNITY_SCHEMA_FIELD_TYPE_REQUIRED);
  unity_schema_add_field (schema, "optional_string", "s", UNITY_SCHEMA_FIELD_TYPE_OPTIONAL);
  return schema;
}

UnityAbstractScope* test_scope_new (const gchar* dbus_path,
                                    UnityCategorySet* category_set,
                                    UnityFilterSet* filter_set,
                                    UnitySimpleScopeSearchRunFunc search_func,
                                    gpointer data)
{
  UnitySimpleScope *scope = unity_simple_scope_new ();

  unity_simple_scope_set_unique_name (scope, dbus_path);
  unity_simple_scope_set_group_name (scope, TEST_DBUS_NAME);
  unity_simple_scope_set_search_hint (scope, "Search hint");

  unity_simple_scope_set_category_set (scope, category_set);
  unity_simple_scope_set_filter_set (scope, filter_set);
  unity_simple_scope_set_schema (scope, test_scope_get_schema ());

  unity_simple_scope_set_search_func (scope, search_func, data, NULL);
  unity_simple_scope_set_preview_func (scope, test_scope_preview, NULL, NULL);
  unity_simple_scope_set_activate_func (scope, test_scope_activate, NULL, NULL);

  return UNITY_ABSTRACT_SCOPE (scope);
}

