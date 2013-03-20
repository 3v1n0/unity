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

  nux::ROProperty<RadioOptions> options;
  nux::ROProperty<bool> show_all_button;

  sigc::signal<void, FilterOption::Ptr> option_added;
  sigc::signal<void, FilterOption::Ptr> option_removed;

protected:
  void Update(Filter::Hints& hints);

private:
  void UpdateState();
  RadioOptions const& get_options() const;
  bool get_show_all_button() const;
  void OptionChanged(bool is_active, std::string const& id);

private:
  RadioOptions options_;
  bool show_all_button_;
  bool ignore_changes_;
};

}
}

#endif
