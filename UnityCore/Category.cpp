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

#include "Category.h"

namespace unity
{
namespace dash
{

Category::Category(DeeModel* model,
                   DeeModelIter* iter,
                   DeeModelTag* renderer_name_tag)
  : RowAdaptorBase(model, iter, renderer_name_tag)
{
  //model_ = model;
  //iter_ = iter;
  //tag_ = renderer_name_tag;

  name.SetGetterFunction(sigc::mem_fun(this, &Category::get_name));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &Category::get_icon_hint));
  index.SetGetterFunction(sigc::mem_fun(this, &Category::get_index));
  renderer_name.SetGetterFunction(sigc::mem_fun(this, &Category::get_renderer_name));
}

Category::Category(Category const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

std::string Category::get_name() const
{
  return dee_model_get_string(model_, iter_, 0);
}

std::string Category::get_icon_hint() const
{
  return dee_model_get_string(model_, iter_, 1);
}

unsigned int Category::get_index() const
{
  return dee_model_get_position(model_, iter_);
}

std::string Category::get_renderer_name() const
{
  return dee_model_get_string(model_, iter_, 2);
}

}
}
