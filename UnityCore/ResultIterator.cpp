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

#include "ResultIterator.h"
#include <NuxCore/Logger.h>
namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.resultiterator");

ResultIterator::ResultIterator(glib::Object<DeeModel> model)
  : model_(model)
  , iter_(model ? dee_model_get_first_iter(model) : NULL)
  , tag_(NULL)
  , iter_result_(model_, iter_, tag_)
{
}

ResultIterator::ResultIterator(glib::Object<DeeModel> model, DeeModelIter* iter, DeeModelTag* tag)
  : model_(model)
  , iter_(iter)
  , tag_(tag)
  , iter_result_(model_, iter_, tag_)
{
}

ResultIterator ResultIterator::operator[](int value)
{
  return ResultIterator(model_, dee_model_get_iter_at_row(model_, value), tag_);
}

ResultIterator& ResultIterator::operator=(ResultIterator const& rhs)
{
  model_ = rhs.model_;
  iter_ = rhs.iter_;
  tag_ = rhs.tag_;
  iter_result_.SetTarget(model_, iter_, tag_);

  return *this;
}

ResultIterator& ResultIterator::operator++()
{
  iter_ = dee_model_next(model_, iter_);
  return *this;
}

ResultIterator& ResultIterator::operator+=(int count)
{
  if (dee_model_is_last(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_next(model_, iter_);
  
  return *this;
}

ResultIterator ResultIterator::operator++(int)
{
  ResultIterator tmp(*this);
  operator++();
  return tmp;
}

ResultIterator ResultIterator::operator+(int count) const
{
  ResultIterator tmp(*this);
  tmp += count;
  return tmp;
}

ResultIterator& ResultIterator::operator--()
{
  iter_ = dee_model_prev(model_, iter_);
  return *this;
}

ResultIterator& ResultIterator::operator-=(int count)
{
  if (dee_model_is_first(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_prev(model_, iter_);

  return *this;
}

ResultIterator ResultIterator::operator--(int)
{
  ResultIterator tmp(*this);
  operator--();
  return tmp;
}

ResultIterator ResultIterator::operator-(int count) const
{
  ResultIterator tmp(*this);
  tmp -= count;
  return tmp;
}

Result& ResultIterator::operator*()
{
  iter_result_.SetTarget(model_, iter_, tag_);
  return iter_result_;
}

bool ResultIterator::IsLast()
{
  if (!model_) return true;
  return (dee_model_is_last(model_, iter_));
}

bool ResultIterator::IsFirst()
{
  if (!model_) return true;
  return (dee_model_is_first(model_, iter_));
}

}
}
