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

#ifndef UNITY_FILTERS_H
#define UNITY_FILTERS_H

#include <boost/shared_ptr.hpp>

#include "Model.h"
#include "Filter.h"

namespace unity {
namespace dash {

class FilterAdaptor;

class Filters : public Model<FilterAdaptor>
{
public:
  typedef boost::shared_ptr<Filters> Ptr;

  Filters();
  ~Filters();

  //sigc::signal<void, Filter const&> filter_added;
  //sigc::signal<void, Filter const&> filter_changed;
  //sigc::signal<void, Filter const&> filter_removed;

private:
  void OnRowAdded(FilterAdaptor& filter);
  void OnRowChanged(FilterAdaptor& filter);
  void OnRowRemoved(FilterAdaptor& filter);
};


}
}

#endif
