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

#ifndef UNITY_CATEGORIES_H
#define UNITY_CATEGORIES_H

#include <memory>

#include "Model.h"
#include "Category.h"

namespace unity
{
namespace dash
{

class Categories : public Model<Category>
{
public:
  typedef std::shared_ptr<Categories> Ptr;

  Categories();
  Categories(ModelType model_type);

  sigc::signal<void, Category const&> category_added;
  sigc::signal<void, Category const&> category_changed;
  sigc::signal<void, Category const&> category_removed;
};

}
}

#endif
