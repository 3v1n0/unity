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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include "ShortcutModel.h"

namespace unity
{
namespace shortcut
{

Model::Model(std::list<AbstractHint::Ptr> const& hints)
  : categories_per_column(3, [] (int& target, int const& new_value) {
      int cat_per_col = std::max<int>(1, new_value);
      if (cat_per_col != target)
      {
        target = cat_per_col;
        return true;
      }
      return false;
    })
{
  for (auto const& hint : hints)
    AddHint(hint);
}

void Model::AddHint(AbstractHint::Ptr const& hint)
{
  if (!hint)
    return;

  if (hints_.find(hint->category()) == hints_.end())
    categories_.push_back(hint->category());

  hints_[hint->category()].push_back(hint);
}

void Model::Fill()
{
  for (auto const& category : categories_)
    for (auto const& item : hints_[category])
      item->Fill();
}

}
}
