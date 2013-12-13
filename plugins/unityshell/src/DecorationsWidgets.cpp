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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <composite/composite.h>
#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{
namespace
{
CompositeScreen* cscreen_ = CompositeScreen::get(screen);
}

Item::Item()
  : max_width_(std::numeric_limits<int>::max())
{}

bool Item::SetMaximumWidth(int max_width)
{
  int clamped = std::max(0, max_width);

  if (max_width_ == clamped)
    return false;

  max_width_ = clamped;
  return true;
}

void Item::Damage()
{
  cscreen_->damageRegion(Geometry());
}

// void Item::SetSize(int width, int height)
// {
//   SetMaximumWidth(width);
// }

//

TexturedItem::TexturedItem()
{
  visible = true;
}

void TexturedItem::Draw(GLWindow* ctx, GLMatrix const& transformation, GLWindowPaintAttrib const& attrib,
                        CompRegion const& clip, unsigned mask)
{
  if (!visible)
    return;

  if (!static_cast<GLTexture*>(texture_))
    UpdateTexture();

  ctx->vertexBuffer()->begin();
  ctx->glAddGeometry({texture_.quad.matrix}, texture_.quad.box, clip);

  if (ctx->vertexBuffer()->end())
    ctx->glDrawTexture(texture_, transformation, attrib, mask);
}

CompRect const& TexturedItem::Geometry() const
{
  return texture_.quad.box;
}

void TexturedItem::SetCoords(int x, int y)
{
  texture_.SetCoords(x, y);
}

void TexturedItem::SetX(int x)
{
  texture_.SetX(x);
}

void TexturedItem::SetY(int y)
{
  texture_.SetY(y);
}

bool TexturedItem::SetMaximumWidth(int max_width)
{
  if (!Item::SetMaximumWidth(max_width))
    return false;

  if (!resizable() && texture_.st && max_width > texture_.st->width())
    max_width = texture_.st->width();

  texture_.quad.box.setWidth(max_width);

  return true;
}

//

Layout::Layout()
  : inner_padding_(0)
{}

void Layout::Append(Item::Ptr const& item)
{
  if (!item || std::find(items_.begin(), items_.end(), item) != items_.end())
    return;

  items_.push_back(item);
  auto relayout_cb = sigc::hide(sigc::mem_fun(this, &Layout::Relayout));
  item->visible.changed.connect(relayout_cb);
  item->resizable.changed.connect(relayout_cb);
  Relayout();
}

CompRect const& Layout::Geometry() const
{
  return rect_;
}

bool Layout::SetMaximumWidth(int max_width)
{
  if (!Item::SetMaximumWidth(max_width))
    return false;

  Relayout();
  return true;
}

void Layout::SetCoords(int x, int y)
{
  if (x == rect_.x() && y == rect_.y())
    return;

  rect_.setX(x);
  rect_.setY(y);
  Relayout();
}

void Layout::Relayout()
{
  bool first_loop = true;

  do
  {
    rect_.setWidth(0);
    rect_.setHeight(0);
    size_t resizable_items = 0;

    for (auto const& item : items_)
    {
      if (!item->visible())
        continue;

      if (first_loop)
        item->SetMaximumWidth(max_width_);

      auto const& item_geo = item->Geometry();
      item->SetX(rect_.x2());
      rect_.setHeight(std::max(rect_.height(), item_geo.height()));

      if (item_geo.width() > 0)
      {
        rect_.setWidth(rect_.width() + item_geo.width() + inner_padding_);

        if (item->resizable())
          ++resizable_items;
      }
    }

    if (!items_.empty())
      rect_.setWidth(std::max(0, rect_.width() - inner_padding_));

    bool too_big = (rect_.width() > max_width_);
    bool has_resizables = (resizable_items > 0);
    int exceeding_width = rect_.width() - max_width_;

    if (first_loop)
    {
      for (auto const& item : items_)
      {
        if (!item->visible())
          continue;

        auto const& item_geo = item->Geometry();
        item->SetY(rect_.y1() + (rect_.height() - item_geo.height()) / 2);

        if (too_big && has_resizables)
        {
          if (item->resizable() && item_geo.width() > 0 && resizable_items > 0)
          {
            int max_item_width = std::max<int>(0, (exceeding_width - item_geo.width()) / resizable_items);
            item->SetMaximumWidth(max_item_width);
            exceeding_width -= item_geo.width();
            --resizable_items;
          }
        }
      }
    }

    if (too_big && (!first_loop || !has_resizables))
    {
      exceeding_width = rect_.width() - max_width_;

      for (auto it = items_.rbegin(); it != items_.rend(); ++it)
      {
        auto const& item = *it;
        if (!item->visible())
          continue;

        int old_width = item->Geometry().width();
        int max_item_width = std::max<int>(0, old_width - exceeding_width);
        item->SetMaximumWidth(max_item_width);
        exceeding_width -= (old_width - max_item_width);

        if (exceeding_width <= 0)
          break;
      }
    }

    first_loop = false;
  }
  while (rect_.width() > max_width_);
}

void Layout::Draw(GLWindow* ctx, GLMatrix const& transformation, GLWindowPaintAttrib const& attrib,
                  CompRegion const& clip, unsigned mask)
{
  for (auto const& item : items_)
  {
    if (item->visible())
      item->Draw(ctx, transformation, attrib, clip, mask);
  }
}

std::list<Item::Ptr> const& Layout::Items() const
{
  return items_;
}

} // decoration namespace
} // unity namespace
