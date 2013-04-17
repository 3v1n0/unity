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
#ifndef UNITY_MODEL_ITERATOR_H
#define UNITY_MODEL_ITERATOR_H

#include <dee.h>
#include <iterator>
#include "GLibWrapper.h"

namespace unity
{
namespace dash
{

// Provides an iterator that will iterate a DeeModel from a Results object
// based on the category you give it

template<class Adaptor>
class ModelIterator : public std::iterator<std::random_access_iterator_tag, Adaptor>
{
public:
  ModelIterator(glib::Object<DeeModel> model);
  ModelIterator(glib::Object<DeeModel> model, DeeModelIter* iter_, DeeModelTag* tag);
  ModelIterator(ModelIterator const& copy) : model_(copy.model_), iter_(copy.iter_), tag_(copy.tag_), iter_result_(copy.iter_result_){};

  ModelIterator& operator=(ModelIterator const& rhs);

  //Iterator methods
  ModelIterator& operator++();
  ModelIterator  operator++(int);
  ModelIterator& operator+=(int value);
  ModelIterator  operator+(int value) const;

  ModelIterator& operator--();
  ModelIterator  operator--(int);
  ModelIterator& operator-=(int value);
  ModelIterator  operator-(int value) const;

  ModelIterator operator[](int value);
  friend inline bool operator<(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return (dee_model_get_position(lhs.model_, lhs.iter_) < dee_model_get_position(rhs.model_, rhs.iter_));
  }

  friend inline bool operator>(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return (dee_model_get_position(lhs.model_, lhs.iter_) > dee_model_get_position(rhs.model_, rhs.iter_));
  }

  friend inline bool operator<=(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return (dee_model_get_position(lhs.model_, lhs.iter_) <= dee_model_get_position(rhs.model_, rhs.iter_));
  }

  friend inline bool operator>=(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return (dee_model_get_position(lhs.model_, lhs.iter_) >= dee_model_get_position(rhs.model_, rhs.iter_));
  }
  
  friend inline bool operator==(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return (lhs.iter_ == rhs.iter_);
  }

  friend inline bool operator!=(const ModelIterator& lhs, const ModelIterator& rhs)
  {
    return !(lhs == rhs);
  }
  Adaptor& operator*();

  /* convenience methods */
  bool IsLast();
  bool IsFirst();

private:
  glib::Object<DeeModel> model_;
  DeeModelIter* iter_;
  DeeModelTag* tag_;
  Adaptor iter_result_;  
};

}
}

#include "ModelIterator-inl.h"

#endif 
