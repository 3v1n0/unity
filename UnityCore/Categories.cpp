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

#include "Categories.h"

namespace unity
{
namespace dash
{

Categories::Categories()
 : Categories(REMOTE)
{
  row_added.connect(sigc::mem_fun(this, &Categories::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Categories::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Categories::OnRowRemoved));
}

Categories::Categories(ModelType model_type)
 : Model<Category>::Model(model_type)
{
  row_added.connect(sigc::mem_fun(this, &Categories::OnRowAdded));
  row_changed.connect(sigc::mem_fun(this, &Categories::OnRowChanged));
  row_removed.connect(sigc::mem_fun(this, &Categories::OnRowRemoved));
}

void Categories::OnRowAdded(Category& category)
{
  category_added.emit(category);
}

void Categories::OnRowChanged(Category& category)
{
  category_changed.emit(category);
}

void Categories::OnRowRemoved(Category& category)
{
  category_removed.emit(category);
}

}
}
