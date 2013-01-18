// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef UNITY_FILTERED_MODEL_INL_H
#define UNITY_FILTERED_MODEL_INL_H

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

template<class RowAdaptor>
FilteredModel<RowAdaptor>::FilteredModel()
: Model<RowAdaptor>(ModelType::UNATTACHED)
{
}

template<class RowAdaptor>
FilteredModel<RowAdaptor>::FilteredModel(std::shared_ptr<Model<RowAdaptor>> const& source_model, DeeFilter* filter)
: Model<RowAdaptor>(ModelType::UNATTACHED)
{
  SetSourceModel(source_model, filter);
}


template<class RowAdaptor>
void FilteredModel<RowAdaptor>::SetSourceModel(std::shared_ptr<Model<RowAdaptor>> const& source_model, DeeFilter* filter)
{
  source_model_ = source_model;
  glib::Object<DeeModel> filter_model(dee_filter_model_new(source_model->model(), filter));

  auto func = [source_model](glib::Object<DeeModel> const& model) { return source_model->GetTag(); };

  SetModel(filter_model, func);
}

} // namespace dash
} // namespace unity

#endif // UNITY_FILTERED_MODEL_INL_H
