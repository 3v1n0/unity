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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef UNITY_MODELITERATOR_INL_H
#define UNITY_MODELITERATOR_INL_H

#include "ModelIterator.h"
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

template<class RowAdaptor>
ModelIterator<RowAdaptor>::ModelIterator(glib::Object<DeeModel> model)
  : model_(model)
  , iter_(model ? dee_model_get_first_iter(model) : NULL)
  , tag_(NULL)
  , iter_result_(model_, iter_, tag_)
{
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>::ModelIterator(glib::Object<DeeModel> model, DeeModelIter* iter, DeeModelTag* tag)
  : model_(model)
  , iter_(iter)
  , tag_(tag)
  , iter_result_(model_, iter_, tag_)
{
}

template<class RowAdaptor>
ModelIterator<RowAdaptor> ModelIterator<RowAdaptor>::operator[](int value)
{
  return ModelIterator(model_, dee_model_get_iter_at_row(model_, value), tag_);
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>& ModelIterator<RowAdaptor>::operator=(ModelIterator const& rhs)
{
  model_ = rhs.model_;
  iter_ = rhs.iter_;
  tag_ = rhs.tag_;
  iter_result_.SetTarget(model_, iter_, tag_);

  return *this;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>& ModelIterator<RowAdaptor>::operator++()
{
  iter_ = dee_model_next(model_, iter_);
  return *this;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>& ModelIterator<RowAdaptor>::operator+=(int count)
{
  if (dee_model_is_last(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_next(model_, iter_);
  
  return *this;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor> ModelIterator<RowAdaptor>::operator++(int)
{
  ModelIterator tmp(*this);
  operator++();
  return tmp;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor> ModelIterator<RowAdaptor>::operator+(int count) const
{
  ModelIterator tmp(*this);
  tmp += count;
  return tmp;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>& ModelIterator<RowAdaptor>::operator--()
{
  iter_ = dee_model_prev(model_, iter_);
  return *this;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor>& ModelIterator<RowAdaptor>::operator-=(int count)
{
  if (dee_model_is_first(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_prev(model_, iter_);

  return *this;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor> ModelIterator<RowAdaptor>::operator--(int)
{
  ModelIterator tmp(*this);
  operator--();
  return tmp;
}

template<class RowAdaptor>
ModelIterator<RowAdaptor> ModelIterator<RowAdaptor>::operator-(int count) const
{
  ModelIterator tmp(*this);
  tmp -= count;
  return tmp;
}

template<class RowAdaptor>
Result& ModelIterator<RowAdaptor>::operator*()
{
  iter_result_.SetTarget(model_, iter_, tag_);
  return iter_result_;
}

template<class RowAdaptor>
bool const ModelIterator<RowAdaptor>::IsLast()
{
  if (!model_) return true;
  return (dee_model_is_last(model_, iter_));
}

template<class RowAdaptor>
bool const ModelIterator<RowAdaptor>::IsFirst()
{
  if (!model_) return true;
  return (dee_model_is_first(model_, iter_));
}

} // namespace dash
} // namespace unity

#endif // UNITY_MODELITERATOR_INL_H

