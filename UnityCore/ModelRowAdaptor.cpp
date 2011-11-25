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

#include "ModelRowAdaptor.h"

namespace unity
{
namespace dash
{
RowAdaptorBase::RowAdaptorBase(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag)
  : model_(model)
  , iter_(iter)
  , tag_(tag)
{}

RowAdaptorBase::RowAdaptorBase(RowAdaptorBase const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

RowAdaptorBase& RowAdaptorBase::operator=(RowAdaptorBase const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;

  return *this;
}

std::string RowAdaptorBase::GetStringAt(int position)
{
  if (!model_ || !iter_)
    return "";
  const gchar* value = dee_model_get_string(model_, iter_, position);
  if (value)
    return value;
  else
    return ""; // std::strings don't like null.
}

bool RowAdaptorBase::GetBoolAt(int position)
{
  if (!model_ || !iter_)
    return 0;
  return dee_model_get_bool(model_, iter_, position);
}

unsigned int RowAdaptorBase::GetUIntAt(int position)
{
  if (!model_ || !iter_)
    return 0;
  return dee_model_get_uint32(model_, iter_, position);
}

}
}
