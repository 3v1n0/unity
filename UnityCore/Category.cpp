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
#include "Variant.h"

#include <sigc++/bind.h>

namespace unity
{
namespace dash
{

enum CategoryColumn
{
  ID = 0,
  DISPLAY_NAME,
  ICON_HINT,
  RENDERER_NAME,
  HINTS
};

Category::Category(DeeModel* model,
                   DeeModelIter* iter,
                   DeeModelTag* renderer_name_tag)
  : RowAdaptorBase(model, iter, renderer_name_tag)
{
  SetupGetters();
}

Category::Category(Category const& other)
  : RowAdaptorBase(other)
{
  SetupGetters();
}

Category& Category::operator=(Category const& other)
{
  RowAdaptorBase::operator=(other);
  SetupGetters();
  return *this;
}

void Category::SetupGetters()
{
  id.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), CategoryColumn::ID));
  name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), CategoryColumn::DISPLAY_NAME));
  icon_hint.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), CategoryColumn::ICON_HINT));
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), CategoryColumn::RENDERER_NAME));
  index.SetGetterFunction(sigc::mem_fun(this, &Category::get_index));
}

std::size_t Category::get_index() const
{
  if (!model_)
    return unsigned(-1);
  return dee_model_get_position(model_, iter_);
}

std::string Category::GetContentType() const
{
  if (!model_ || !iter_)
    return "";

  glib::HintsMap hints;
  glib::Variant v(dee_model_get_value(model_, iter_, CategoryColumn::HINTS),
                  glib::StealRef());
  v.ASVToHints(hints);
  auto iter = hints.find("content-type");
  if (iter == hints.end())
    return "";
  return iter->second.GetString();
}

}
}
