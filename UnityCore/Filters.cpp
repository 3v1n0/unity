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

Filters::Filters()
{
  row_added.connect(sigc::mem_fun(this, &Filters::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Filters::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Filters::OnRowRemoved));
}

Filters::~Filters()
{}

void Filters::OnRowAdded(FilterAdaptor& filter)
{}

void Filters::OnRowChanged(FilterAdaptor& filter)
{}

void Filters::OnRowRemoved(FilterAdaptor& filter)
{}

}
}
