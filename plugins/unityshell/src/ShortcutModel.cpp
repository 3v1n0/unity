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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include "ShortcutModel.h"

namespace unity
{
namespace shortcut
{

// Ctor
Model::Model(std::list<AbstractHint::Ptr>& hints)
{
  for (auto hint : hints)
    AddHint(hint);
}

// Dtor
Model::~Model()
{
}


void Model::AddHint(AbstractHint::Ptr hint)
{
  if (!hint)
    return;

  if (hints_.find(hint->category()) == hints_.end())
    categories_.push_back(hint->category());

  hints_[hint->category()].push_back(hint);
}


void Model::Fill()
{
  for (auto category : categories_)
    for (auto item : hints_[category])
      item->Fill();
}

} // namespace shortcut
} // namespace unity
