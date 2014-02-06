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

MenuLayout::MenuLayout()
  : active(false)
{}

void MenuLayout::Setup(AppmenuIndicator::Ptr const& appmenu, CompWindow* win)
{
  items_.clear();
  auto ownership_cb = sigc::mem_fun(this, &MenuLayout::OnEntryMouseOwnershipChanged);
  auto active_cb = sigc::mem_fun(this, &MenuLayout::OnEntryActiveChanged);

  for (auto const& entry : appmenu->GetEntries())
  {
    auto en = std::make_shared<MenuEntry>(entry, win);
    en->mouse_owner.changed.connect(ownership_cb);
    en->active.changed.connect(active_cb);
    Append(en);
  }
}

void MenuLayout::OnEntryMouseOwnershipChanged(bool owner)
{
  mouse_owner = owner;
}

void MenuLayout::OnEntryActiveChanged(bool actived)
{
  active = actived;

  if (active && !pointer_tracker_ && Items().size() > 1)
  {
    pointer_tracker_.reset(new glib::Timeout(16));
    pointer_tracker_->Run([this] {

      Window win;
      int i, x, y;
      unsigned int ui;

      XQueryPointer(screen->dpy(), screen->root(), &win, &win, &x, &y, &i, &i, &ui);

      if (last_pointer_.x() != x || last_pointer_.y() != y)
      {
        last_pointer_.set(x, y);

        for (auto const& item : Items())
        {
          if (!item->visible())
            continue;

          if (item->Geometry().contains(last_pointer_))
          {
            std::static_pointer_cast<MenuEntry>(item)->ShowMenu(1);
            break;
          }
        }
      }

      return true;
    });
  }
  else if (!active)
  {
    pointer_tracker_.reset();
  }
}

void MenuLayout::ChildrenGeometries(EntryLocationMap& map) const
{
  for (auto const& item : Items())
  {
    if (item->visible())
    {
      auto const& entry = std::static_pointer_cast<MenuEntry>(item);
      auto const& geo = item->Geometry();
      map.insert({entry->Id(), {geo.x(), geo.y(), geo.width(), geo.height()}});
    }
  }
}

} // decoration namespace
} // unity namespace
