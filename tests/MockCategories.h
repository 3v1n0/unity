// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef _UNITY_MOCK_CATEGORIES_H
#define _UNITY_MOCK_CATEGORIES_H

#include <UnityCore/Categories.h>

namespace unity
{
namespace dash
{

struct MockCategories : public Categories
{
  MockCategories(unsigned count_)
    : Categories(LOCAL)
  {
    count.SetGetterFunction([count_] { return count_; });
  }
};

// Template specialization for Category in tests
template<>
const Category Model<Category>::RowAtIndex(std::size_t index) const
{
  Category mock_category(nullptr, nullptr, nullptr);
  mock_category.id.SetGetterFunction([index] { return "cat"+std::to_string(index); });
  mock_category.name.SetGetterFunction([index] { return "Category "+std::to_string(index); });
  mock_category.icon_hint.SetGetterFunction([] { return "cmake"; });
  mock_category.renderer_name.SetGetterFunction([] { return "grid"; });
  mock_category.index.SetGetterFunction([index] { return index; });
  return mock_category;
}

}
}

#endif // _UNITY_MOCK_CATEGORIES_H