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

#include "DecorationsSlidingLayout.h"

namespace unity
{
namespace decoration
{

SlidingLayout::SlidingLayout()
{
  items_.resize(2);
  mouse_owner.changed.connect([this] (bool) { Damage(); });
}

void SlidingLayout::SetMainItem(Item::Ptr const& main)
{
  if (main_item_ == main)
    return;

  if (main_item_)
    main_item_->SetParent(nullptr);

  main_item_ = main;
  items_[1] = main;

  if (main_item_)
    main_item_->SetParent(shared_from_this());

  Relayout();
}

void SlidingLayout::SetInputItem(Item::Ptr const& input)
{
  if (input_item_ == input)
    return;

  if (input_item_)
    input_item_->SetParent(nullptr);

  input_item_ = input;
  items_[0] = input;

  if (input_item_)
    input_item_->SetParent(shared_from_this());

  Relayout();
}

void SlidingLayout::DoRelayout()
{
  nux::Size contents;

  for (auto const& item : items_)
  {
    if (!item || !item->visible())
      continue;

    item->SetX(rect_.x());
    item->SetMinWidth(item->GetNaturalWidth());
    item->SetMaxWidth(max_.width);
    item->SetMinHeight(item->GetNaturalHeight());
    item->SetMaxHeight(max_.height);

    auto const& geo = item->Geometry();
    contents.width = std::max(contents.width, geo.width());
    contents.height = std::max(contents.height, geo.height());
  }

  for (auto const& item : items_)
  {
    if (!item || !item->visible())
      continue;

    item->SetY(rect_.y() + (contents.height - item->Geometry().height())/2);
  }

  rect_.setWidth(contents.width);
  rect_.setHeight(contents.height);
}

void SlidingLayout::Draw(GLWindow* ctx, GLMatrix const& transformation, GLWindowPaintAttrib const& attrib, CompRegion const& clip, unsigned mask)
{
  auto const& draw_area = (mouse_owner() && input_item_) ? input_item_ : main_item_;
  draw_area->Draw(ctx, transformation, attrib, clip, mask);
}

} // decoration namespace
} // unity namespace
