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

namespace unity
{
namespace dash
{

namespace
{
enum ResultColumn
{
  URI,
  ICON_HINT,
  CATEGORY,
  RESULT_TYPE,
  MIMETYPE,
  TITLE,
  COMMENT,
  DND_URI,
  METADATA
};
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

Result& Result::operator=(Result const& other)
{
  RowAdaptorBase::operator=(other);
  SetupGetters();
  return *this;
}

void Result::SetupGetters()
{
  uri.SetGetterFunction(sigc::mem_fun(this, &Result::GetURI));
  icon_hint.SetGetterFunction(sigc::mem_fun(this, &Result::GetIconHint));
  category_index.SetGetterFunction(sigc::mem_fun(this, &Result::GetCategoryIndex));
  result_type.SetGetterFunction(sigc::mem_fun(this, &Result::GetResultType));
  mimetype.SetGetterFunction(sigc::mem_fun(this, &Result::GetMimeType));
  name.SetGetterFunction(sigc::mem_fun(this, &Result::GetName));
  comment.SetGetterFunction(sigc::mem_fun(this, &Result::GetComment));
  dnd_uri.SetGetterFunction(sigc::mem_fun(this, &Result::GetDndURI));
}

std::string Result::GetURI() const { return GetStringAt(ResultColumn::URI); }
std::string Result::GetIconHint() const { return GetStringAt(ResultColumn::ICON_HINT); }
std::size_t Result::GetCategoryIndex() const { return GetUIntAt(ResultColumn::CATEGORY); }
unsigned Result::GetResultType() const { return GetUIntAt(ResultColumn::RESULT_TYPE); }
std::string Result::GetMimeType() const { return GetStringAt(ResultColumn::MIMETYPE); }
std::string Result::GetName() const { return GetStringAt(ResultColumn::TITLE); }
std::string Result::GetComment() const { return GetStringAt(ResultColumn::COMMENT); }
std::string Result::GetDndURI() const { return GetStringAt(ResultColumn::DND_URI); }

}
}
