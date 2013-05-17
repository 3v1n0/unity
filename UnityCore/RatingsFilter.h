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

#ifndef UNITY_RATINGS_FILTER_H
#define UNITY_RATINGS_FILTER_H

#include "Filter.h"

namespace unity
{
namespace dash
{

class RatingsFilter : public Filter
{
public:
  typedef std::shared_ptr<RatingsFilter> Ptr;

  RatingsFilter(DeeModel* model, DeeModelIter* iter);

  void Clear();

  nux::Property<float> rating;
  nux::ROProperty<bool> show_all_button;

protected:
  void Update(Filter::Hints& hints);

private:
  void OnRatingChanged(float new_value);
  void UpdateState(float new_rating);

  bool get_show_all_button() const;
  bool show_all_button_;
};

}
}

#endif
