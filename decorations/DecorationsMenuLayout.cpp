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
#include "DecorationsMenuDropdown.h"

namespace unity
{
namespace decoration
{
namespace
{
const unsigned MENU_SHOW_NOW_WAIT = 180;
}

using namespace indicator;

MenuLayout::MenuLayout(Indicators::Ptr const& indicators, CompWindow* win)
  : active(false)
  , show_now(false)
  , win_(win)
  , dropdown_(std::make_shared<MenuDropdown>(indicators, win))
{}

void MenuLayout::SetAppMenu(AppmenuIndicator::Ptr const& appmenu)
{
  items_.clear();
  auto ownership_cb = sigc::mem_fun(this, &MenuLayout::OnEntryMouseOwnershipChanged);
  auto active_cb = sigc::mem_fun(this, &MenuLayout::OnEntryActiveChanged);
  auto show_now_cb = sigc::mem_fun(this, &MenuLayout::OnEntryShowNowChanged);

  dropdown_->mouse_owner.changed.connect(ownership_cb);
  dropdown_->active.changed.connect(active_cb);
  dropdown_->show_now.changed.connect(show_now_cb);

  for (auto const& entry : appmenu->GetEntries())
  {
    auto menu = std::make_shared<MenuEntry>(entry, win_);
    menu->mouse_owner.changed.connect(ownership_cb);
    menu->active.changed.connect(active_cb);
    menu->show_now.changed.connect(show_now_cb);
    menu->focused = focused();
    menu->SetParent(shared_from_this());
    items_.push_back(menu);
  }

  if (!items_.empty())
    Relayout();
}

void MenuLayout::ActivateMenu(std::string const& entry_id)
{
  MenuEntry::Ptr target;
  bool activated = false;

  for (auto const& item : items_)
  {
    auto const& menu_entry = std::static_pointer_cast<MenuEntry>(item);

    if (menu_entry->Id() == entry_id)
    {
      target = menu_entry;

      if (item->visible() && item->sensitive())
      {
        menu_entry->ShowMenu(0);
        activated = true;
      }

      break;
    }
  }

  if (!activated)
    dropdown_->ActivateChild(target);
}

void MenuLayout::OnEntryMouseOwnershipChanged(bool owner)
{
  mouse_owner = owner;
}

void MenuLayout::OnEntryShowNowChanged(bool show)
{
  if (!show)
  {
    show_now_timeout_.reset();
    show_now = false;
  }
  else
  {
    show_now_timeout_.reset(new glib::Timeout(MENU_SHOW_NOW_WAIT));
    show_now_timeout_->Run([this] {
      show_now = true;
      show_now_timeout_.reset();
      return false;
    });
  }
}

void MenuLayout::OnEntryActiveChanged(bool actived)
{
  active = actived;

  if (active && !pointer_tracker_ && items_.size() > 1)
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

        for (auto const& item : items_)
        {
          if (item->Geometry().contains(last_pointer_))
          {
            if (item->visible() && item->sensitive())
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
  for (auto const& item : items_)
  {
    if (item->visible())
    {
      auto const& entry = std::static_pointer_cast<MenuEntry>(item);
      auto const& geo = item->Geometry();
      map.insert({entry->Id(), {geo.x(), geo.y(), geo.width(), geo.height()}});
    }
  }
}

void MenuLayout::DoRelayout()
{
  int dropdown_width = dropdown_->GetNaturalWidth();
  int accumolated_width = dropdown_width + left_padding() + right_padding() - inner_padding();
  int max_width = max_.width;
  std::list<MenuEntry::Ptr> to_hide;

  for (auto const& item : items_)
  {
    if (!item->visible() || item == dropdown_)
      continue;

    accumolated_width += item->GetNaturalWidth() + inner_padding();

    if (accumolated_width > max_width)
      to_hide.push_front(std::static_pointer_cast<MenuEntry>(item));
  }

  // No need to hide an item if there's space that we considered for the dropdown
  if (dropdown_->Empty() && to_hide.size() == 1)
  {
    if (accumolated_width - dropdown_width < max_width)
      to_hide.clear();
  }

  // There's just one hidden entry, it might fit in the space we have
  if (to_hide.empty() && dropdown_->Size() == 1)
    accumolated_width -= dropdown_width;

  if (accumolated_width < max_width)
  {
    while (!dropdown_->Empty() && dropdown_->Top()->GetNaturalWidth() < (max_width - accumolated_width))
      dropdown_->Pop();

    if (dropdown_->Empty())
      Remove(dropdown_);
  }
  else if (!to_hide.empty())
  {
    if (dropdown_->Empty())
      Append(dropdown_);

    for (auto const& hidden : to_hide)
      dropdown_->Push(hidden);
  }

  Layout::DoRelayout();
}

} // decoration namespace
} // unity namespace
