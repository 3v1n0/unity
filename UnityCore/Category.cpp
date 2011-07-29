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

#include <sigc++/bind.h>

namespace unity
{
namespace dash
{

Category::Category(DeeModel* model,
                   DeeModelIter* iter,
                   DeeModelTag* renderer_name_tag)
  : RowAdaptorBase(model, iter, renderer_name_tag)
{
  SetupGetters();
}

Category::Category(Category const& other)
  : RowAdaptorBase(other.model_, other.iter_, other.tag_)
{
  SetupGetters();
}

void Category::SetupGetters()
{
  name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 0));
  icon_hint.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 1));
  index.SetGetterFunction(sigc::mem_fun(this, &Category::get_index));
  renderer_name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 2));
}

std::size_t Category::get_index() const
{
  return dee_model_get_position(model_, iter_);
}

}
}
