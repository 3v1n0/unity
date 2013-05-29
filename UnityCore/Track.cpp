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

#include "Track.h"
#include <sigc++/bind.h>

namespace unity
{
namespace dash
{

Track::Track(DeeModel* model,
             DeeModelIter* iter,
             DeeModelTag* renderer_tag)
  : RowAdaptorBase(model, iter, renderer_tag)
{
  SetupGetters();
}

Track::Track(Track const& other)
  : RowAdaptorBase(other)
{
  SetupGetters();
}

Track& Track::operator=(Track const& other)
{
  RowAdaptorBase::operator=(other);
  SetupGetters();
  return *this;
}

void Track::SetupGetters()
{
  uri.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 0));
  track_number.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetIntAt), 1));
  title.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 2));
  length.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetUIntAt), 3));
  index.SetGetterFunction(sigc::mem_fun(this, &Track::get_index));
}

std::size_t Track::get_index() const
{
  if (!model_)
    return unsigned(-1);
  return dee_model_get_position(model_, iter_);
}

}
}
