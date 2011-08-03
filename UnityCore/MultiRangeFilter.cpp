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

#include "MultiRangeFilter.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.multirangefilter");
}

MultiRangeFilter::MultiRangeFilter(DeeModel* model, DeeModelIter* iter)
  : Filter(model, iter)
  , left_pos_(-1)
  , right_pos_(-1)
{
  segments.SetGetterFunction(sigc::mem_fun(this, &MultiRangeFilter::get_segments));
  Refresh();
}

void MultiRangeFilter::Clear()
{
  left_pos_ = -1;
  right_pos_ = -1;

  for(auto segment: segments_)
  {
    segment->active = false;
  }
  UpdateState(false);

  segments.EmitChanged(segments_);
}

void MultiRangeFilter::Toggle(std::string const& id)
{
  int position = PositionOfId(id);

  if (left_pos_ == -1 && right_pos_ == -1)
  {
    left_pos_ = position;
    right_pos_ = position;
  }
  else if (left_pos_ > position)
  {
    left_pos_ = position;
  }
  else if (right_pos_ < position)
  {
    right_pos_ = position;
  }

  int i = 0;
  for(auto segment: segments_)
  {
    if (i < left_pos_)
      segment->active = false;
    else if (i <= right_pos_)
      segment->active = true;
    else
      segment->active = false;

    i++;
  }
  UpdateState(true);

  segments.EmitChanged(segments_);
}

void MultiRangeFilter::Update(Filter::Hints& hints)
{
  GVariant* segments_variant = hints["segments"];
  GVariantIter* segments_iter;

  g_variant_get(segments_variant, "(sssb)", &segments_iter);

  char *id = NULL;
  char *name = NULL;
  char *icon_hint = NULL;
  gboolean active = false;

  for (auto segment: segments_)
    segment_removed.emit(segment);
  segments_.clear();

  while (g_variant_iter_loop(segments_iter, "sssb", &id, &name, &icon_hint, &active))
  {
    FilterButton::Ptr segment(new FilterButton(id, name, icon_hint, active));
    segments_.push_back(segment);
    segment_added.emit(segment);
  }

  segments.EmitChanged(segments_);
}

MultiRangeFilter::Segments const& MultiRangeFilter::get_segments() const
{
  return segments_;
}

void MultiRangeFilter::UpdateState(bool raw_filtering)
{
  if (!IsValid())
    return;

  GVariantBuilder segments;
  g_variant_builder_init(&segments, G_VARIANT_TYPE("(sssb)"));

  for(auto segment: segments_)
  {
    std::string id = segment->id;
    std::string name = segment->name;
    std::string icon_hint = segment->icon_hint;
    bool active = segment->active;

    g_variant_builder_add(&segments, "sssb",
                          id.c_str(), name.c_str(),
                          icon_hint.c_str(), active ? TRUE : FALSE);
  }
  
  GVariantBuilder hints;
  g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&hints, "{sv}", "segments", g_variant_builder_end(&segments));

  dee_model_set_value(model_,iter_,
                      FilterColumn::RENDERER_STATE,
                      g_variant_builder_end(&hints));
  dee_model_set_value(model_, iter_,
                      FilterColumn::FILTERING,
                      g_variant_new("b", raw_filtering ? TRUE : FALSE));
}

int MultiRangeFilter::PositionOfId(std::string const& id)
{
  int i = 0;
  for (auto segment: segments_)
  {
    if (segment->id == id)
      return i;
    i++;
  }
  return -1;
}


}
}
