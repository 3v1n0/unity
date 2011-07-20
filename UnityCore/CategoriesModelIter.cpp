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

#include "CategoriesModelIter.h"

#include <NuxCore/Logger.h>

namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.resultsmodeliter");
}

CategoriesModelIter::CategoriesModelIter(DeeModel* model,
                                         DeeModelIter* iter,
                                         DeeModelTag* renderer_tag)
{
  model_ = model;
  iter_ = iter;
  tag_ = renderer_tag;

  name.SetGetterFunction(sigc::mem_fun(this, &CategoriesModelIter::get_name));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &CategoriesModelIter::get_icon_hint));
  index.SetGetterFunction(sigc::mem_fun(this, &CategoriesModelIter::get_index));
  renderer.SetGetterFunction(sigc::mem_fun(this, &CategoriesModelIter::get_renderer));
}

CategoriesModelIter::CategoriesModelIter(CategoriesModelIter const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

std::string CategoriesModelIter::get_name() const
{
  return dee_model_get_string(model_, iter_, 0);
}

std::string CategoriesModelIter::get_icon_hint() const
{
  return dee_model_get_string(model_, iter_, 1);
}

unsigned int CategoriesModelIter::get_index() const
{
  return dee_model_get_position(model_, iter_);
}

std::string CategoriesModelIter::get_renderer() const
{
  return dee_model_get_string(model_, iter_, 2);
}

}
}
