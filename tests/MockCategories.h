// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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

#ifndef _UNITY_MOCK_CATEGORIES_H
#define _UNITY_MOCK_CATEGORIES_H

#include <dee.h>

namespace unity
{
namespace dash
{

class MockCategories : public Categories
{
public:
MockCategories(unsigned count)
: Categories(LOCAL)
, model_(dee_sequence_model_new())
{
  dee_model_set_schema(model_, "s", "s", "s", "s", "a{sv}", nullptr);
  AddResults(count);

  SetModel(model_);
}

void AddResults(unsigned count)
{
  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  GVariant *hints = g_variant_builder_end(&b);

  for(unsigned i = 0; i < count; ++i)
  {
    dee_model_append(model_,
                     ("cat"+std::to_string(i)).c_str(),
                     ("Category "+std::to_string(i)).c_str(),
                     "gtk-apply",
                     "grid",
                     hints);
  }
  g_variant_unref(hints);
}

glib::Object<DeeModel> model_;
};

}
}

#endif // _UNITY_MOCK_CATEGORIES_H