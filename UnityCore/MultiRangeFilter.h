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

#ifndef UNITY_MULTI_RANGE_FILTER_H
#define UNITY_MULTI_RANGE_FILTER_H

#include "Filter.h"

namespace unity
{
namespace dash
{

class MultiRangeFilter : public Filter
{
public:
  typedef std::shared_ptr<MultiRangeFilter> Ptr;
  typedef std::vector<FilterButton::Ptr> Buttons;

  MultiRangeFilter(DeeModel* model, DeeModelIter* iter);

  void Clear();

  /* When a button is clicked. Don't worry about state, we'll handle that internally.
   * From this you'll get a changed event on "buttons", which you can then re-render
   * your view. The logic of the multi-range is handled internally.
   */
  void Toggle(std::string const& id);

  nux::ROProperty<Buttons> buttons;

  sigc::signal<void, FilterButton::Ptr> button_added;
  sigc::signal<void, FilterButton::Ptr> button_removed;

protected:
  void Update(Filter::Hints& hints);

private:
  void UpdateState(bool raw_filtering);
  Buttons const& get_buttons() const;
  int PositionOfId(std::string const& id);

private:
  Buttons buttons_;
  int left_pos_;
  int right_pos_;
};

}
}

#endif
