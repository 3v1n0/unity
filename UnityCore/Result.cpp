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

#include "Result.h"

namespace unity {
namespace dash {

Result::Result(DeeModel* model,
               DeeModelIter* iter,
               DeeModelTag* renderer_tag)
{
  model_ = model;
  iter_ = iter;
  tag_ = renderer_tag;

  uri.SetGetterFunction(sigc::mem_fun(this, &Result::get_uri));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &Result::get_icon_hint));
  category_index.SetGetterFunction(sigc::mem_fun(this, &Result::get_category));
  mimetype.SetGetterFunction(sigc::mem_fun(this, &Result::get_mimetype));
  name.SetGetterFunction(sigc::mem_fun(this, &Result::get_name));
  comment.SetGetterFunction(sigc::mem_fun(this, &Result::get_comment));
  dnd_uri.SetGetterFunction(sigc::mem_fun(this, &Result::get_dnd_uri));
}

Result::Result(Result const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

std::string Result::get_uri() const
{
  return dee_model_get_string(model_, iter_, 0);
}

std::string Result::get_icon_hint() const
{
  return dee_model_get_string(model_, iter_, 1);
}

unsigned int Result::get_category() const
{
  return dee_model_get_uint32(model_, iter_, 2);
}

std::string Result::get_mimetype() const
{
  return dee_model_get_string(model_, iter_, 3);
}

std::string Result::get_name() const
{
  return dee_model_get_string(model_, iter_, 4);
}

std::string Result::get_comment() const
{
  return dee_model_get_string(model_, iter_, 5);
}

std::string Result::get_dnd_uri() const
{
  return dee_model_get_string(model_, iter_, 6);
}

}
}
