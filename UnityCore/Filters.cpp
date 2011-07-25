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

#include "Filters.h"

namespace unity
{
namespace dash
{

class FilterAdaptor : public RowAdaptorBase
{
public:
  FilterAdaptor(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);

  FilterAdaptor(FilterAdaptor const& other);

  nux::ROProperty<std::string> id;
  nux::ROProperty<std::string> renderer_name;
private:
  std::string get_id() const;
  std::string get_renderer_name() const;
};

FilterAdaptor::FilterAdaptor(DeeModel* model,
                             DeeModelIter* iter,
                             DeeModelTag* renderer_tag)
{
  model_ = model;
  iter_ = iter;
  tag_ = renderer_tag;

  renderer_name.SetGetterFunction(sigc::mem_fun(this, &FilterAdaptor::get_renderer_name));
}

FilterAdaptor::FilterAdaptor(FilterAdaptor const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

std::string FilterAdaptor::get_renderer_name() const
{
  return dee_model_get_string(model_, iter_, 3);
}

std::string FilterAdaptor::get_id() const
{
  return dee_model_get_string(model_, iter_, 0);
}

Filters::Filters()
{
  row_added.connect(sigc::mem_fun(this, &Filters::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Filters::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Filters::OnRowRemoved));
}

Filters::~Filters()
{}

void Filters::OnRowAdded(FilterAdaptor& filter)
{
  //filter_added.emit(filter);
}

void Filters::OnRowChanged(FilterAdaptor& filter)
{
  //filter_changed.emit(filter);
}

void Filters::OnRowRemoved(FilterAdaptor& filter)
{
  //filter_removed.emit(filter);
}

}
}
