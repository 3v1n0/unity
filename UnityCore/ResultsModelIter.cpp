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

#include "ResultsModelIter.h"

#include <NuxCore/Logger.h>

namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.resultsmodeliter");
}

ResultsModelIter::ResultsModelIter(DeeModel* model,
                                   DeeModelIter* iter,
                                   DeeModelTag* renderer_tag)
  : model_(model)
  , iter_(iter)
  , tag_(renderer_tag)
{
  uri.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_uri));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_icon_hint));
  category.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_category));
  mimetype.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_mimetype));
  name.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_name));
  comment.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_comment));
  dnd_uri.SetGetterFunction(sigc::mem_fun(this, &ResultsModelIter::get_dnd_uri));
}

ResultsModelIter::ResultsModelIter(ResultsModelIter const& other)
{
  model_ = other.model_;
  iter_ = other.iter_;
  tag_ = other.tag_;
}

template<typename T>
void ResultsModelIter::set_renderer(T renderer)
{
  dee_model_set_tag(model_, iter_, tag_, renderer);
}

template<typename T>
T ResultsModelIter::renderer()
{
  return static_cast<T>(dee_model_get_tag(model_, iter_, tag_));
}

std::string ResultsModelIter::get_uri() const
{
  return dee_model_get_string(model_, iter_, 0);
}

std::string ResultsModelIter::get_icon_hint() const
{
  return dee_model_get_string(model_, iter_, 1);
}

unsigned int ResultsModelIter::get_category() const
{
  return dee_model_get_uint32(model_, iter_, 2);
}

std::string ResultsModelIter::get_mimetype() const
{
  return dee_model_get_string(model_, iter_, 3);
}

std::string ResultsModelIter::get_name() const
{
  return dee_model_get_string(model_, iter_, 4);
}

std::string ResultsModelIter::get_comment() const
{
  return dee_model_get_string(model_, iter_, 5);
}

std::string ResultsModelIter::get_dnd_uri() const
{
  return dee_model_get_string(model_, iter_, 6);
}

}
}
