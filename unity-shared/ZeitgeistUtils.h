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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_ZEITGEIST_UTILS
#define UNITY_ZEITGEIST_UTILS

#include <zeitgeist.h>

/* Unfortunately valac-generated zeitgeist library contains invalid definitions
 * that makes impossible for us to compile with -Werror -Wall, so we need to
 * workaround them.
 * Unfortunately using #pragma doesn't help much here as we can't limit the
 * errors to the header inclusion only, so it's better to use this way in order
 * to be able to include zeitgeist in multiple places without ignoring warnings.
 */

static ZeitgeistEvent* zeitgeist_event_constructv_full(GType object_type, const gchar* interpretation, const gchar* manifestation, const gchar* actor, const gchar* origin, va_list _vala_va_list)
{
  return nullptr;
}

namespace unity
{
namespace zeitgeist
{
namespace workaround
{
namespace
{
void ___dummy_function()
{
  (void) zeitgeist_event_constructv_full;
}
} // anonymous namespace
} // workaround namespace
} // zeitgeist namespace
} // unity namespace

#endif // UNITY_ZEITGEIST_UTILS