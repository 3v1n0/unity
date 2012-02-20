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

namespace 
{
  nux::logging::Logger logger("unity.dash.resultiterator");
}

ResultIterator::ResultIterator(glib::Object<DeeModel> model)
  : model_(model, glib::AddRef())
  , iter_(dee_model_get_first_iter(model))
  , iter_result_(model_, iter_, NULL)
  , cache_invalidated_(false)
{
}

ResultIterator& ResultIterator::operator++()
{
  iter_ = dee_model_next(model_, iter_);
  cache_invalidated_ = true;
  return *this;
}

ResultIterator& ResultIterator::operator+(int count)
{
  if (dee_model_is_last(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_next(model_, iter_);
  
  cache_invalidated_ = true;
  return *this;
}

ResultIterator& ResultIterator::operator--()
{
  iter_ = dee_model_prev(model_, iter_);
  cache_invalidated_ = true;
  return *this;
}

ResultIterator& ResultIterator::operator-(int count)
{
  if (dee_model_is_first(model_, iter_))
    return *this;

  for (int index = 0; index < count; index++)
    iter_ = dee_model_prev(model_, iter_);

  cache_invalidated_ = true;
  return *this;
}

bool ResultIterator::operator==(const ResultIterator& rhs)
{
  GVariant* lhsv = dee_model_get_value(model_, iter_, 0);
  GVariant* rhsv = dee_model_get_value(rhs.model_, rhs.iter_, 0);
  bool equality = g_variant_equal(lhsv, rhsv);
  g_variant_unref(lhsv);
  g_variant_unref(rhsv);
  return equality;
}

bool ResultIterator::operator!=(const ResultIterator& rhs)
{
  GVariant* lhsv = dee_model_get_value(model_, iter_, 0);
  GVariant* rhsv = dee_model_get_value(rhs.model_, rhs.iter_, 0);
  bool equality = g_variant_equal(lhsv, rhsv);
  g_variant_unref(lhsv);
  g_variant_unref(rhsv);
  return !equality;
}

Result const& ResultIterator::operator*()
{
  //if (cache_invalidated_)
    iter_result_ = Result(model_, iter_, NULL);
  return iter_result_;
}

bool ResultIterator::IsLast()
{
  return (dee_model_is_last(model_, iter_));
}

bool ResultIterator::IsFirst()
{
  return (dee_model_is_first(model_, iter_));
}

}
}
