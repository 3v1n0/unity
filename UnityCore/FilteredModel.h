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

#ifndef UNITY_FILTERED_MODEL_H
#define UNITY_FILTERED_MODEL_H

#include <sigc++/trackable.h>

#include "Model.h"

namespace unity
{
namespace dash
{

template<class RowAdaptor>
class FilteredModel : public Model<RowAdaptor>
{
public:
  typedef std::shared_ptr<FilteredModel> Ptr;

  FilteredModel();
  FilteredModel(std::shared_ptr<Model<RowAdaptor>> const& source_model, DeeFilter* filter);

  void SetSourceModel(std::shared_ptr<Model<RowAdaptor>> const& source_model, DeeFilter* filter);

private:
  using Model<RowAdaptor>::SetModel;

private:
  std::shared_ptr<Model<RowAdaptor>> source_model_;
};

} // namespace dash
} // namespace unity

#include "FilteredModel-inl.h"

#endif // UNITY_FILTERED_MODEL_H