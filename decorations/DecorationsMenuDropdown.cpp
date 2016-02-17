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

#include "DecorationsMenuDropdown.h"
#include "DecorationStyle.h"
#include "unity-shared/ThemeSettings.h"

namespace unity
{
namespace decoration
{
namespace
{
const std::string ICON_NAME = "go-down-symbolic";
const unsigned ICON_SIZE = 10;
}

using namespace indicator;

MenuDropdown::MenuDropdown(Indicators::Ptr const& indicators, CompWindow* win)
  : MenuEntry(std::make_shared<Entry>("LIM"+std::to_string(win->id())+"-dropdown"), win)
  , indicators_(indicators)
{
  natural_.width = ICON_SIZE;
  natural_.height = ICON_SIZE;
  entry_->set_image(1, ICON_NAME, true, true);
  theme::Settings::Get()->icons_changed.connect(sigc::mem_fun(this, &MenuDropdown::RenderTexture));
}

void MenuDropdown::ShowMenu(unsigned button)
{
  if (active)
   return;

  active = true;
  auto const& geo = Geometry();
  Indicator::Entries entries;

  for (auto const& child : children_)
    entries.push_back(child->GetEntry());

  indicators_->ShowEntriesDropdown(entries, active_, grab_.Window()->id(), geo.x(), geo.y2());
}

bool MenuDropdown::ActivateChild(MenuEntry::Ptr const& child)
{
  if (!child || std::find(children_.begin(), children_.end(), child) == children_.end())
    return false;

  active_ = child->GetEntry();
  ShowMenu(0);
  active_.reset();
  return true;
}

void MenuDropdown::Push(MenuEntry::Ptr const& child)
{
  if (!child)
    return;

  if (std::find(children_.begin(), children_.end(), child) != children_.end())
    return;

  int size_diff = (child->GetNaturalHeight() - GetNaturalHeight()) / scale();

  if (size_diff > 0)
  {
    natural_.height += (size_diff % 2);
    vertical_padding = vertical_padding() + (size_diff / 2);
  }

  children_.push_front(child);
  child->GetEntry()->add_parent(entry_);
  child->in_dropdown = true;
}

MenuEntry::Ptr MenuDropdown::Pop()
{
  if (children_.empty())
    return nullptr;

  auto child = children_.front();
  child->GetEntry()->rm_parent(entry_);
  child->in_dropdown = false;
  children_.pop_front();

  return child;
}

MenuEntry::Ptr MenuDropdown::Top() const
{
  return (!children_.empty()) ? children_.front() : nullptr;
}

size_t MenuDropdown::Size() const
{
  return children_.size();
}

bool MenuDropdown::Empty() const
{
  return children_.empty();
}

void MenuDropdown::RenderTexture()
{
  WidgetState normal_state = focused() ? WidgetState::NORMAL : WidgetState::BACKDROP;
  WidgetState state = active() ? WidgetState::PRELIGHT : normal_state;
  cu::CairoContext icon_ctx(GetNaturalWidth(), GetNaturalHeight(), scale());

  if (state == WidgetState::PRELIGHT)
    Style::Get()->DrawMenuItem(state, icon_ctx, icon_ctx.width() / scale(), icon_ctx.height() / scale());

  cairo_save(icon_ctx);
  cairo_translate(icon_ctx, horizontal_padding(), vertical_padding());
  cairo_save(icon_ctx);
  cairo_scale(icon_ctx, 1.0f/scale(), 1.0f/scale());
  Style::Get()->DrawMenuItemIcon(ICON_NAME, state, icon_ctx, ICON_SIZE * scale());
  cairo_restore(icon_ctx);
  cairo_restore(icon_ctx);
  SetTexture(icon_ctx);
}

debug::Introspectable::IntrospectableList MenuDropdown::GetIntrospectableChildren()
{
  IntrospectableList list;

  for (auto const& child : children_)
    list.push_back(child.get());

  return list;
}

} // decoration namespace
} // unity namespace
