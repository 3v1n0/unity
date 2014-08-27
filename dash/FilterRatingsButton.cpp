/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include "FilterRatingsButton.h"

namespace
{
const int STAR_SIZE = 28;
const int STAR_GAP  = 10;
}

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterRatingsButton);

FilterRatingsButton::FilterRatingsButton(NUX_FILE_LINE_DECL)
  : RatingsButton(STAR_SIZE, STAR_GAP, NUX_FILE_LINE_PARAM)
{}

void FilterRatingsButton::SetFilter(Filter::Ptr const& filter)
{
  filter_ = std::static_pointer_cast<RatingsFilter>(filter);
  filter_->rating.changed.connect(sigc::mem_fun(this, &FilterRatingsButton::SetRating));
  QueueDraw();
}

RatingsFilter::Ptr FilterRatingsButton::GetFilter() const
{
  return filter_;
}

std::string FilterRatingsButton::GetFilterType()
{
  return "FilterRatingsButton";
}

std::string FilterRatingsButton::GetName() const
{
  return "FilterRatingsButton";
}

void FilterRatingsButton::SetRating(float rating)
{
  if (filter_)
    filter_->rating = rating;

  QueueDraw();
}

float FilterRatingsButton::GetRating() const
{
  return (filter_ && filter_->filtering) ? filter_->rating : 0;
}

} // namespace dash
} // namespace unity
