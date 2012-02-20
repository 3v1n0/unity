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
#ifndef UNITY_RESULT_ITERATOR_H
#define UNITY_RESULT_ITERATOR_H

#include <dee.h>
#include <memory>
#include <iterator>
#include "Result.h"
#include "Results.h"

namespace unity
{
namespace dash
{

// Provides an iterator that will iterate a DeeModel from a Results object
// based on the category you give it

class ResultIterator : public std::iterator<std::input_iterator_tag, Result>
{
public:
  ResultIterator(glib::Object<DeeModel> model);
  ResultIterator(ResultIterator const& copy) : model_(copy.model_), iter_(copy.iter_), iter_result_(copy.iter_result_), cache_invalidated_(false){};
  //Iterator methods
  ResultIterator& operator++();
  ResultIterator& operator+(int value);
  ResultIterator& operator--();
  ResultIterator& operator-(int value);
  bool operator==(const ResultIterator& rhs);
  bool operator!=(const ResultIterator& rhs);
  Result const& operator*();

  /* convenience methods */
  bool IsLast();
  bool IsFirst();

private:
  glib::Object<DeeModel> model_;
  DeeModelIter* iter_;
  Result iter_result_;
  bool cache_invalidated_;
};

}

}

#endif 
