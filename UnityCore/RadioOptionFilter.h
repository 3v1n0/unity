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

#ifndef UNITY_RADIO_OPTION_FILTER_H
#define UNITY_RADIO_OPTION_FILTER_H

#include "Filter.h"

namespace unity
{
namespace dash
{

class RadioOptionFilter : public Filter
{
public:
  typedef std::shared_ptr<RadioOptionFilter> Ptr;
  typedef std::vector<FilterOption::Ptr> RadioOptions;

  RadioOptionFilter(DeeModel* model, DeeModelIter* iter);

  void Clear();

  /* When a option is clicked. Don't worry about state, we'll handle that internally.
   * From this you'll get a changed event on the options effected,
   * which you can then re-render your view. The logic of the radio option is handled
   * internally
   */
  void Toggle(std::string id);

  nux::ROProperty<RadioOptions> options;

  sigc::signal<void, FilterOption::Ptr> option_added;
  sigc::signal<void, FilterOption::Ptr> option_removed;

protected:
  void Update(Filter::Hints& hints);

private:
  void UpdateState(bool raw_filtering);
  RadioOptions const& get_options() const;

private:
  RadioOptions options_;
};

}
}

#endif
