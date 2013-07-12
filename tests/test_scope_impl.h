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

UnityAbstractScope* test_scope_new (const gchar* dbus_path,
                                    UnityCategorySet* category_set,
                                    UnityFilterSet* filter_set,
                                    UnitySimpleScopeSearchRunFunc search_func,
                                    gpointer user_data);

G_END_DECLS

#endif /* _TEST_SCOPE_IMPL_H_ */
