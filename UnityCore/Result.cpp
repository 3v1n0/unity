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
#include <sigc++/bind.h>
#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{
namespace
{
nux::logging::Logger logger("unity.dash");
}

Result::Result(DeeModel* model,
               DeeModelIter* iter,
               DeeModelTag* renderer_tag)
  : RowAdaptorBase(model, iter, renderer_tag)
{
  SetupGetters();
}

Result::Result(Result const& other)
  : RowAdaptorBase(other)
{
  SetupGetters();
}

Result::~Result()
{
}

Result& Result::operator=(Result const& other)
{
  RowAdaptorBase::operator=(other);
  SetupGetters();
  return *this;
}

void Result::SetupGetters()
{
  uri.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 0));
  icon_hint.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 1));
  category_index.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetUIntAt), 2));
  mimetype.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 3));
  name.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 4));
  comment.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 5));
  dnd_uri.SetGetterFunction(sigc::bind(sigc::mem_fun(this, &RowAdaptorBase::GetStringAt), 6));
}

}
}
