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

#define _g_object_unref_safe(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free_safe(var) (var = (g_free (var), NULL))

/* ------------------ Test Searcher ---------------------- */

#define TEST_TYPE_SEARCHER (test_searcher_get_type ())
#define TEST_SCOPE_SEARCHER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_SEARCHER, TestSearcher))
#define TEST_SEARCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_SEARCHER, TestSearcherClass))
#define TEST_IS_SEARCHER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_SEARCHER))
#define TEST_IS_SEARCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TEST_TYPE_SEARCHER))
#define TEST_SEARCHER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_SEARCHER, TestSearcherClass))

typedef struct _TestSearcher TestSearcher;
typedef struct _TestSearcherClass TestSearcherClass;
typedef struct _TestSearcherPrivate TestSearcherPrivate;

struct _TestSearcher {
  UnityScopeSearchBase parent_instance;
  TestSearcherPrivate * priv;
};

struct _TestSearcherClass {
  UnityScopeSearchBaseClass parent_class;
};

struct _TestSearcherPrivate {
  TestScope* _owner;
};

typedef struct _SearcherRunData SearcherRunData;

G_DEFINE_TYPE(TestSearcher, test_searcher, UNITY_TYPE_SCOPE_SEARCH_BASE);

#define TEST_SEARCHER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TEST_TYPE_SEARCHER, TestSearcherPrivate))
enum  {
  TEST_SEARCHER_DUMMY_PROPERTY,
  TEST_SEARCHER_OWNER
};

static TestSearcher* test_searcher_new (TestScope* scope);
static void test_searcher_run (UnityScopeSearchBase* base);
static TestScope* test_searcher_get_owner (TestSearcher* self);
static void test_searcher_set_owner (TestSearcher* self, TestScope* value);
static void test_searcher_finalize (GObject* obj);
static void test_searcher_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void test_searcher_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);

static TestSearcher* test_searcher_new (TestScope* scope)
{
  return (TestSearcher*) g_object_new (TEST_TYPE_SEARCHER, "owner", scope, NULL);
}

static TestScope* test_searcher_get_owner (TestSearcher* self)
{
  g_return_val_if_fail (self != NULL, NULL);
  return self->priv->_owner;
}

static void test_searcher_set_owner (TestSearcher* self, TestScope* value)
{
  g_return_if_fail (self != NULL);
  self->priv->_owner = value;
  g_object_notify ((GObject *) self, "owner");
}

static void test_searcher_class_init (TestSearcherClass * klass) 
{
  g_type_class_add_private (klass, sizeof (TestSearcherPrivate));
  UNITY_SCOPE_SEARCH_BASE_CLASS (klass)->run = test_searcher_run;
  G_OBJECT_CLASS (klass)->get_property = test_searcher_get_property;
  G_OBJECT_CLASS (klass)->set_property = test_searcher_set_property;
  G_OBJECT_CLASS (klass)->finalize = test_searcher_finalize;
  g_object_class_install_property (G_OBJECT_CLASS (klass), TEST_SEARCHER_OWNER, g_param_spec_object ("owner", "owner", "owner", TEST_TYPE_SCOPE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void test_searcher_init(TestSearcher* self)
{
  self->priv = TEST_SEARCHER_GET_PRIVATE (self);
}

static void test_searcher_finalize (GObject* obj)
{
  TestSearcher * self;
  self = TEST_SCOPE_SEARCHER (obj);
  G_OBJECT_CLASS (test_searcher_parent_class)->finalize (obj);
}

static void test_searcher_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  TestSearcher * self;
  self = TEST_SCOPE_SEARCHER (object);
  switch (property_id) {
    case TEST_SEARCHER_OWNER:
    g_value_set_object (value, test_searcher_get_owner (self));
    break;
    default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void test_searcher_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  TestSearcher * self;
  self = TEST_SCOPE_SEARCHER (object);
  switch (property_id) {
    case TEST_SEARCHER_OWNER:
    test_searcher_set_owner (self, g_value_get_object (value));
    break;
    default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

struct _SearcherRunData 
{
  int _ref_count_;
  TestSearcher * self;
  GMainLoop* ml;
  UnityScopeSearchBaseCallback async_callback;
  void* async_callback_target;
};

static SearcherRunData* run_data_ref (SearcherRunData* data)
{
  g_atomic_int_inc (&data->_ref_count_);
  return data;
}

static void run_data_unref (void * _userdata_)
{
  SearcherRunData* data;
  data = (SearcherRunData*) _userdata_;
  if (g_atomic_int_dec_and_test (&data->_ref_count_))
  {
    TestSearcher* self;
    self = data->self;
    g_main_loop_unref (data->ml);
    g_object_unref (self);
    g_slice_free (SearcherRunData, data);
  }
}

static gboolean test_searcher_main_loop_func (gpointer data)
{
  SearcherRunData* search_data;
  search_data = (SearcherRunData*) data;

  TestSearcher* self;
  self = search_data->self;
  UNITY_SCOPE_SEARCH_BASE_GET_CLASS (self)->run(UNITY_SCOPE_SEARCH_BASE (self));

  search_data->async_callback (UNITY_SCOPE_SEARCH_BASE (self), search_data->async_callback_target);
  g_main_loop_quit (search_data->ml);
  return FALSE;
}

static void test_searcher_run (UnityScopeSearchBase* base)
{
  TestSearcher* self;
  self = TEST_SCOPE_SEARCHER (base);

  g_signal_emit_by_name (self->priv->_owner, "search", base);
}

/* ------------------ Test Result Previewer ---------------------- */

#define TEST_TYPE_RESULT_PREVIEWER (test_result_previewer_get_type ())
#define TEST_RESULT_PREVIEWER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_RESULT_PREVIEWER, TestResultPreviewer))
#define TEST_RESULT_PREVIEWER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_RESULT_PREVIEWER, TestResultPreviewerClass))
#define TEST_IS_RESULT_PREVIEWER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_RESULT_PREVIEWER))
#define TEST_IS_RESULT_PREVIEWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TEST_TYPE_RESULT_PREVIEWER))
#define TEST_RESULT_PREVIEWER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_RESULT_PREVIEWER, TestResultPreviewerClass))

typedef struct _TestResultPreviewer TestResultPreviewer;
typedef struct _TestResultPreviewerClass TestResultPreviewerClass;

struct _TestResultPreviewer {
  UnityResultPreviewer parent_instance;
};

struct _TestResultPreviewerClass {
  UnityResultPreviewerClass parent_class;
};

G_DEFINE_TYPE(TestResultPreviewer, test_result_previewer, UNITY_TYPE_RESULT_PREVIEWER);

static UnityAbstractPreview* test_result_previewer_run(UnityResultPreviewer* self)
{
  UnityAbstractPreview* preview;
  UnityPreviewAction* action;
  preview = UNITY_ABSTRACT_PREVIEW (unity_generic_preview_new ("title", "description", NULL));

  action = unity_preview_action_new ("action1", "Action 1", NULL);
  unity_preview_add_action(UNITY_PREVIEW (preview), action);

  action = unity_preview_action_new ("action2", "Action 2", NULL);
  // GHashTable* hints = unity_preview_action_get_hints(action);

  unity_preview_add_action(UNITY_PREVIEW (preview), action);

  return preview;
}

static void test_result_previewer_class_init(TestResultPreviewerClass* klass)
{
  UNITY_RESULT_PREVIEWER_CLASS (klass)->run = test_result_previewer_run;
}

static void test_result_previewer_init(TestResultPreviewer* self)
{
}

TestResultPreviewer* test_result_previewer_new ()
{
  return (TestResultPreviewer*) g_object_new (TEST_TYPE_RESULT_PREVIEWER, NULL);
}

/* ------------------ Test Scope ---------------------- */

#define TEST_SCOPE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TEST_TYPE_SCOPE, TestScopePrivate))

G_DEFINE_TYPE(TestScope, test_scope, UNITY_TYPE_ABSTRACT_SCOPE);

struct _TestScopePrivate
{
  gchar* _dbus_path;
  UnityScopeDBusConnector* _connector;
  UnityCategorySet* _categories;
  UnityFilterSet* _filters;
};

enum  {
  TEST_SCOPE_DUMMY_PROPERTY,
  TEST_SCOPE_DBUS_PATH
};

static UnityScopeSearchBase* test_scope_create_search_for_query(UnityAbstractScope* self, UnitySearchContext* search_context);
static UnityResultPreviewer* test_scope_create_previewer(UnityAbstractScope* self, UnityScopeResult* _result_, UnitySearchMetadata* metadata);
static UnityCategorySet* test_scope_get_categories(UnityAbstractScope* self);
static UnityFilterSet* test_scope_get_filters(UnityAbstractScope* self);
static UnitySchema* test_scope_get_schema(UnityAbstractScope* self);
static gchar* test_scope_get_search_hint(UnityAbstractScope* self);
static gchar* test_scope_get_group_name(UnityAbstractScope* self);
static gchar* test_scope_get_unique_name(UnityAbstractScope* self);
static UnityActivationResponse* test_scope_activate(UnityAbstractScope* self, UnityScopeResult* _result_, UnitySearchMetadata* metadata, const gchar* action_id);
static void test_scope_set_dbus_path (TestScope* self, const gchar* value);
static const gchar* test_scope_get_dbus_path (TestScope* self);

static void test_scope_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void test_scope_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void g_cclosure_user_marshal_OBJECT__STRING (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data);

static void test_scope_class_init(TestScopeClass* klass)
{
  g_type_class_add_private (klass, sizeof (TestScopePrivate));

  UNITY_ABSTRACT_SCOPE_CLASS (klass)->create_search_for_query = test_scope_create_search_for_query;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->create_previewer = test_scope_create_previewer;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_categories = test_scope_get_categories;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_filters = test_scope_get_filters;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_schema = test_scope_get_schema;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_search_hint = test_scope_get_search_hint;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_group_name = test_scope_get_group_name;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->get_unique_name = test_scope_get_unique_name;
  UNITY_ABSTRACT_SCOPE_CLASS (klass)->activate = test_scope_activate;
  G_OBJECT_CLASS (klass)->get_property = test_scope_get_property;
  G_OBJECT_CLASS (klass)->set_property = test_scope_set_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), TEST_SCOPE_DBUS_PATH, g_param_spec_string ("dbus-path", "dbus-path", "dbus-path", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
  g_signal_new ("search", TEST_TYPE_SCOPE, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, UNITY_TYPE_SCOPE_SEARCH_BASE);
  g_signal_new ("activate_uri", TEST_TYPE_SCOPE, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_OBJECT__STRING, UNITY_TYPE_ACTIVATION_RESPONSE, 1, G_TYPE_STRING);
}

static void test_scope_init(TestScope* self)
{
  self->priv = TEST_SCOPE_GET_PRIVATE (self);

  self->priv->_dbus_path = NULL;
  self->priv->_connector = NULL;
  self->priv->_categories = NULL;
  self->priv->_filters = NULL;
}

static void test_scope_finalize (GObject* obj)
{
  TestScope * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, TEST_TYPE_SCOPE, TestScope);
  _g_free_safe (self->priv->_dbus_path);
  _g_object_unref_safe (self->priv->_connector);
  _g_object_unref_safe (self->priv->_categories);
  G_OBJECT_CLASS (test_scope_parent_class)->finalize (obj);
}

static void test_scope_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  TestScope * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, TEST_TYPE_SCOPE, TestScope);
  switch (property_id)
  {
    case TEST_SCOPE_DBUS_PATH:
    g_value_set_string (value, test_scope_get_dbus_path (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void test_scope_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  TestScope * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, TEST_TYPE_SCOPE, TestScope);
  switch (property_id) {
    case TEST_SCOPE_DBUS_PATH:
    test_scope_set_dbus_path (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static UnityScopeSearchBase* test_scope_create_search_for_query(UnityAbstractScope* base, UnitySearchContext* search_context)
{
  UnityScopeSearchBase* searcher;
  TestScope* scope;
  scope = (TestScope*) base;
  searcher = (UnityScopeSearchBase*) test_searcher_new (scope);

  UnitySearchContext ctx;
  ctx = *search_context;
  unity_scope_search_base_set_search_context (searcher, &ctx);
  return searcher;
}

static UnityResultPreviewer* test_scope_create_previewer(UnityAbstractScope* self, UnityScopeResult* result, UnitySearchMetadata* metadata)
{
  UnityResultPreviewer* previewer;
  previewer = UNITY_RESULT_PREVIEWER (test_result_previewer_new());
  unity_scope_result_copy(result, &previewer->result);
  return previewer;
}

static UnityCategorySet* test_scope_get_categories(UnityAbstractScope* base)
{
  TestScope* self;
  self = G_TYPE_CHECK_INSTANCE_CAST (base, TEST_TYPE_SCOPE, TestScope);
  g_return_if_fail (self != NULL);

  return self->priv->_categories;
}

static UnityFilterSet* test_scope_get_filters(UnityAbstractScope* base)
{
  TestScope* self;
  self = G_TYPE_CHECK_INSTANCE_CAST (base, TEST_TYPE_SCOPE, TestScope);
  g_return_if_fail (self != NULL);

  return self->priv->_filters;
}

static UnitySchema* test_scope_get_schema(UnityAbstractScope* self)
{
  UnitySchema* schema = unity_schema_new ();
  unity_schema_add_field (schema, "required_string", "s", UNITY_SCHEMA_FIELD_TYPE_REQUIRED);
  unity_schema_add_field (schema, "required_int", "i", UNITY_SCHEMA_FIELD_TYPE_REQUIRED);
  unity_schema_add_field (schema, "optional_string", "s", UNITY_SCHEMA_FIELD_TYPE_OPTIONAL);
  return schema;
}

static gchar* test_scope_get_search_hint(UnityAbstractScope* self)
{
  return g_strdup ("Search hint");
}

static gchar* test_scope_get_group_name(UnityAbstractScope* self)
{
  return g_strdup (TEST_DBUS_NAME);
}

static gchar* test_scope_get_unique_name(UnityAbstractScope* base)
{
  TestScope * self;
  self = (TestScope*) base;
  return g_strdup (self->priv->_dbus_path);
}

static UnityActivationResponse* test_scope_activate(UnityAbstractScope* self, UnityScopeResult* result, UnitySearchMetadata* metadata, const gchar* action_id)
{
  UnityActivationResponse* response = NULL;
  g_signal_emit_by_name (self, "activate-uri", result->uri, &response);
  return response;
}

static const gchar* test_scope_get_dbus_path (TestScope* self)
{
  g_return_val_if_fail (self != NULL, NULL);
  return self->priv->_dbus_path;
}

static void test_scope_set_dbus_path (TestScope* self, const gchar* value)
{
  g_return_if_fail (self != NULL);
  _g_free_safe (self->priv->_dbus_path);
  self->priv->_dbus_path = g_strdup (value);;
  g_object_notify ((GObject *) self, "dbus-path");
}

static void g_cclosure_user_marshal_OBJECT__STRING (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data)
{
  typedef gpointer (*GMarshalFunc_OBJECT__STRING) (gpointer data1, const char* arg_1, gpointer data2);
  register GMarshalFunc_OBJECT__STRING callback;
  register GCClosure * cc;
  register gpointer data1;
  register gpointer data2;
  gpointer v_return;
  cc = (GCClosure *) closure;
  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);
  if (G_CCLOSURE_SWAP_DATA (closure)) {
    data1 = closure->data;
    data2 = param_values->data[0].v_pointer;
  } else {
    data1 = param_values->data[0].v_pointer;
    data2 = closure->data;
  }
  callback = (GMarshalFunc_OBJECT__STRING) (marshal_data ? marshal_data : cc->callback);
  v_return = callback (data1, g_value_get_string (param_values + 1), data2);
  g_value_take_object (return_value, v_return);
}

TestScope* test_scope_new (const gchar* dbus_path)
{
  return (TestScope*) g_object_new (TEST_TYPE_SCOPE, "dbus-path", dbus_path, NULL);
}

void test_scope_set_categories(TestScope* self, UnityCategorySet* categories)
{
  g_return_if_fail (self != NULL);

  g_object_ref(categories);
  self->priv->_categories = categories;
}

void test_scope_set_filters(TestScope* self, UnityFilterSet* filters)
{
  g_return_if_fail (self != NULL);

  g_object_ref(filters);
  self->priv->_filters = filters;
}

void test_scope_export (TestScope* self, GError** error)
{
  UnityScopeDBusConnector* connector;
  g_return_if_fail (self != NULL);

  connector = unity_scope_dbus_connector_new ((UnityAbstractScope*) self);
  _g_object_unref_safe (self->priv->_connector);
  self->priv->_connector = connector;
  unity_scope_dbus_connector_export (connector, error);
}

void test_scope_unexport (TestScope* self)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (self->priv->_connector != NULL);
  unity_scope_dbus_connector_unexport (self->priv->_connector);
}