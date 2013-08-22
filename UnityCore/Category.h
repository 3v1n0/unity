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

#ifndef UNITY_CATEGORY_H
#define UNITY_CATEGORY_H

#include <NuxCore/Property.h>

#include "ModelRowAdaptor.h"

namespace unity
{
namespace dash
{

/* This class represents a DeeModelIter for a CategoriesModel  */
class Category : public RowAdaptorBase
{
public:
  Category(DeeModel* model, DeeModelIter* iter, DeeModelTag* tag);

  Category(Category const& other);
  Category& operator=(Category const& other);
  
  nux::ROProperty<std::string> id;
  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> icon_hint;
  nux::ROProperty<std::string> renderer_name;
  nux::ROProperty<std::size_t> index;

  std::string GetContentType() const;

private:
  void SetupGetters();
  std::size_t get_index() const;
};

}
}

#endif
