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

FilterAdaptor::FilterAdaptor(DeeModel* model,
                             DeeModelIter* iter,
                             DeeModelTag* renderer_tag)
  : RowAdaptorBase(model, iter, renderer_tag)
{
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 3));
}

FilterAdaptor::FilterAdaptor(FilterAdaptor const& other)
  : RowAdaptorBase(other.model_, other.iter_, other.tag_)
{
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 3));
}

DeeModel* FilterAdaptor::model() const
{
  return model_;
}

DeeModelIter* FilterAdaptor::iter() const
{
  return iter_;
}


Filters::Filters()
: Model<FilterAdaptor>::Model(ModelType::REMOTE)
{
  row_added.connect(sigc::mem_fun(this, &Filters::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Filters::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Filters::OnRowRemoved));
}

Filters::Filters(ModelType model_type)
: Model<FilterAdaptor>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(this, &Filters::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Filters::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Filters::OnRowRemoved));
}

Filters::~Filters()
{}

Filter::Ptr Filters::FilterAtIndex(std::size_t index)
{
  FilterAdaptor adaptor = RowAtIndex(index);
  if (filter_map_.find(adaptor.iter()) == filter_map_.end())
  {
    OnRowAdded(adaptor);
  }
  return filter_map_[adaptor.iter()];
}

void Filters::OnRowAdded(FilterAdaptor& filter)
{
  Filter::Ptr ret = Filter::FilterFromIter(filter.model(), filter.iter());

  filter_map_[filter.iter()] = ret;
  filter_added(ret);
}

void Filters::OnRowChanged(FilterAdaptor& filter)
{
  filter_changed(filter_map_[filter.iter()]);
}

void Filters::OnRowRemoved(FilterAdaptor& filter)
{
  filter_removed(filter_map_[filter.iter()]);
  filter_map_.erase(filter.iter());
}

}
}
