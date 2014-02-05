// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "DecorationsMenuLayout.h"
#include "DecorationsMenuEntry.h"

namespace unity
{
namespace decoration
{
using namespace indicator;

void MenuLayout::Setup(AppmenuIndicator::Ptr const& appmenu, CompWindow* win)
{
  items_.clear();
  auto ownership_cb = sigc::mem_fun(this, &MenuLayout::OnEntryMouseOwnershipChanged);

  for (auto const& entry : appmenu->GetEntries())
  {
    auto en = std::make_shared<MenuEntry>(entry, win);
    en->mouse_owner.changed.connect(ownership_cb);
    Append(en);
  }
}

void MenuLayout::OnEntryMouseOwnershipChanged(bool owner)
{
  mouse_owner = owner;
}

} // decoration namespace
} // unity namespace
