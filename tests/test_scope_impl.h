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

#ifndef _TEST_SCOPE_IMPL_H_
#define _TEST_SCOPE_IMPL_H_

#include <glib-object.h>
#include <unity.h>

G_BEGIN_DECLS

#define TEST_TYPE_SCOPE                  (test_scope_get_type ())
#define TEST_SCOPE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TEST_TYPE_SCOPE, TestScope))
#define TEST_IS_SCOPE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TEST_TYPE_SCOPE))
#define TEST_SCOPE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), TEST_TYPE_SCOPE, TestScopeClass))
#define TEST_IS_SCOPE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), TEST_TYPE_SCOPE))
#define TEST_SCOPE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), TEST_TYPE_SCOPE, TestScopeClass))

typedef struct _TestScope        TestScope;
typedef struct _TestScopeClass   TestScopeClass;
typedef struct _TestScopePrivate TestScopePrivate;

struct _TestScope
{
  UnityAbstractScope parent_instance;

  /* instance members */
  TestScopePrivate * priv;
};

struct _TestScopeClass
{
  UnityAbstractScopeClass parent_class;

  /* class members */
};

/* used by TEST_TYPE_SCOPE */
GType test_scope_get_type (void);

TestScope* test_scope_new (const gchar* dbus_path);

void test_scope_set_categories(TestScope* self, UnityCategorySet* categories);
void test_scope_set_filters(TestScope* self, UnityFilterSet* filters);
void test_scope_export (TestScope* self, GError** error);
void test_scope_unexport (TestScope* self);

G_END_DECLS

#endif /* _TEST_SCOPE_IMPL_H_ */