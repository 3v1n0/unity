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

#include <memory>
#include <map>

#include "Model.h"
#include "ModelIterator.h"
#include "Filter.h"

namespace unity
{
namespace dash
{

class FilterAdaptor : public RowAdaptorBase
{
public:
  FilterAdaptor(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);
  FilterAdaptor(FilterAdaptor const&);

  nux::ROProperty<std::string> renderer_name;

  std::string get_id() const;
  std::string get_name() const;
  std::string get_icon_hint() const;
  std::string get_renderer_name() const;
  bool get_visible() const;
  bool get_collapsed() const;
  bool get_filtering() const;

  Filter::Ptr create_filter() const;

  void MergeState(glib::HintsMap const& hints);
};

typedef ModelIterator<FilterAdaptor> FilterAdaptorIterator;

class Filters : public Model<FilterAdaptor>
{
public:
  typedef std::shared_ptr<Filters> Ptr;
  typedef std::map<std::string, Filter::Ptr> FilterMap;

  Filters();
  Filters(ModelType model_type);
  ~Filters();

  Filter::Ptr FilterAtIndex(std::size_t index);

  sigc::signal<void, Filter::Ptr> filter_added;
  sigc::signal<void, Filter::Ptr> filter_changed;
  sigc::signal<void, Filter::Ptr> filter_removed;

  bool ApplyStateChanges(glib::Variant const& filter_rows);

  FilterAdaptorIterator begin() const;
  FilterAdaptorIterator end() const;

  /* There will be added/changed/removed signals here when we have that working */
private:
  void OnRowAdded(FilterAdaptor& filter);
  void OnRowChanged(FilterAdaptor& filter);
  void OnRowRemoved(FilterAdaptor& filter);

  FilterMap filter_map_;
};


}
}

#endif
