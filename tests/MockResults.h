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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_MOCK_RESULTS_H
#define UNITY_MOCK_RESULTS_H

#include "UnityCore/Results.h"

namespace unity {
namespace dash {

struct MockResults : public Results
{
  MockResults(unsigned int count)
    : Results(LOCAL)
    , model_(dee_sequence_model_new())
  {
    dee_model_set_schema(model_, "s", "s", "u", "u", "s", "s", "s", "s", "a{sv}", nullptr);
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
      dee_model_append(model_, "uri", "icon_hint", 
                       0, 0, "mimetype", 
                       "title", "comment", "dnd_uri", hints);
    }

    g_variant_unref(hints);
  }

  glib::Object<DeeModel> model_;
};

}
}

#endif