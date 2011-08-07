// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "RatingsFilter.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.ratingsfilter");
}

RatingsFilter::RatingsFilter(DeeModel* model, DeeModelIter* iter)
  : Filter(model, iter)
{
  rating.changed.connect(sigc::mem_fun(this, &RatingsFilter::OnRatingChanged));
  Refresh();
}

void RatingsFilter::Clear()
{
  UpdateState(0.0f, false);
}

void RatingsFilter::Update(Filter::Hints& hints)
{
  GVariant* rating_variant = hints["rating"];

  if (rating_variant)
  {
    rating = (float)g_variant_get_double(rating_variant);
  }
  else
  {
    LOG_WARN(logger) << "Hints do not contain a 'rating' key";
    rating = 0.0f;
  }
}

void RatingsFilter::OnRatingChanged(float new_value)
{
  UpdateState(new_value, true);
}

void RatingsFilter::UpdateState(float raw_rating, bool raw_filtering)
{
  if (!IsValid())
    return;

  GVariantBuilder b;
  g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&b, "{sv}", "rating", g_variant_new("d", raw_rating));

  dee_model_set_value(model_,iter_,
                      FilterColumn::RENDERER_STATE,
                      g_variant_builder_end(&b));
  dee_model_set_value(model_, iter_,
                      FilterColumn::FILTERING,
                      g_variant_new("b", raw_filtering ? TRUE : FALSE));
}

}
}
