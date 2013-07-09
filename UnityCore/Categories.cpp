// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2013 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "Categories.h"

namespace unity
{
namespace dash
{

Categories::Categories()
 : Categories(REMOTE)
{}

Categories::Categories(ModelType model_type)
 : Model<Category>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(&category_added, &decltype(category_added)::emit));
  row_changed.connect(sigc::mem_fun(&category_changed, &decltype(category_changed)::emit));
  row_removed.connect(sigc::mem_fun(&category_removed, &decltype(category_removed)::emit));
}

}
}
