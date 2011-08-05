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

#ifndef UNITY_CHECK_OPTION_FILTER_H
#define UNITY_CHECK_OPTION_FILTER_H

#include "Filter.h"

namespace unity
{
namespace dash
{

class CheckOptionFilter : public Filter
{
public:
  typedef std::shared_ptr<CheckOptionFilter> Ptr;

  typedef std::vector<FilterOption::Ptr> CheckOptions;

  CheckOptionFilter(DeeModel* model, DeeModelIter* iter);

  void Clear();

  /* When a option is clicked. Don't worry about state, we'll handle that internally.
   * From this you'll get a changed event on "options", which you can then re-render
   * your view
   */
  void Toggle(std::string id);

  nux::ROProperty<CheckOptions> options;

  sigc::signal<void, FilterOption::Ptr> option_added;
  sigc::signal<void, FilterOption::Ptr> option_removed;

protected:
  void Update(Filter::Hints& hints);

private:
  void UpdateState(bool raw_filtering);
  CheckOptions const& get_options() const;

private:
  CheckOptions options_;
};

}
}

#endif
