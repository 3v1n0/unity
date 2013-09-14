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

RowAdaptorBase::~RowAdaptorBase()
{
}

RowAdaptorBase& RowAdaptorBase::operator=(RowAdaptorBase const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;

  return *this;
}

void RowAdaptorBase::SetTarget(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag)
{
  model_ = model;
  iter_ = iter;
  tag_ = tag;
}

std::string RowAdaptorBase::GetStringAt(int position) const
{
  if (!model_ || !iter_)
    return "";
  const gchar* value = dee_model_get_string(model_, iter_, position);
  if (value)
    return value;
  else
    return ""; // std::strings don't like null.
}

bool RowAdaptorBase::GetBoolAt(int position) const
{
  if (!model_ || !iter_)
    return 0;
  return dee_model_get_bool(model_, iter_, position);
}

int RowAdaptorBase::GetIntAt(int position) const
{
  if (!model_ || !iter_)
    return 0;
  return dee_model_get_int32(model_, iter_, position);
}

unsigned int RowAdaptorBase::GetUIntAt(int position) const
{
  if (!model_ || !iter_)
    return 0;
  return dee_model_get_uint32(model_, iter_, position);
}

float RowAdaptorBase::GetFloatAt(int position) const
{
  if (!model_ || !iter_)
    return 0.0;
  return static_cast<float>(dee_model_get_double(model_, iter_, position));
}

glib::Variant RowAdaptorBase::GetVariantAt(int position) const
{
  if (!model_ || !iter_)
    return nullptr;
  return dee_model_get_value(model_, iter_, position);
}

void RowAdaptorBase::set_model_tag(gpointer value)
{
  dee_model_set_tag(model_, iter_, tag_, value);
}

gpointer RowAdaptorBase::get_model_tag() const
{
  return dee_model_get_tag(model_, iter_, tag_);
}

}
}
