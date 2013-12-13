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
#include <boost/range/adaptor/reversed.hpp>
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
  : max_(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())
{}

void Item::SetSize(int width, int height)
{
  SetMinWidth(width);
  SetMaxWidth(width);
  SetMinHeight(height);
  SetMaxHeight(height);
  natural_.width = width;
  natural_.height = height;
}

void Item::SetCoords(int x, int y)
{
  if (Geometry().x() == x || Geometry.y() == y)
    return;

  InternalGeo().setX(x);
  InternalGeo().setY(y);
}

int Item::GetNaturalWidth() const
{
  return natural_.width;
}

int Item::GetNaturalHeight() const
{
  return natural_.height;
}

bool Item::SetMaxWidth(int value)
{
  int clamped = std::max(0, value);

  if (max_.width == clamped)
    return false;

  max_.width = clamped;
  min_.width = std::min(min_.width, max_.width);

  if (Geometry().width() > max_.width)
    InternalGeo().setWidth(std::min(GetNaturalWidth(), max_.width));

  return true;
}

bool Item::SetMinWidth(int value)
{
  int clamped = std::max(0, value);

  if (min_.width == clamped)
    return false;

  min_.width = clamped;
  max_.width = std::max(min_.width, max_.width);

  if (Geometry().width() < min_.width)
    InternalGeo().setWidth(min_.width);

  return true;
}

bool Item::SetMaxHeight(int value)
{
  int clamped = std::max(0, value);

  if (max_.height == clamped)
    return false;

  max_.height = clamped;
  min_.height = std::min(min_.height, max_.height);

  if (Geometry().height() > max_.height)
    InternalGeo().setHeight(std::min(GetNaturalWidth(), max_.height));

  return true;
}

bool Item::SetMinHeight(int value)
{
  int clamped = std::max(0, value);

  if (min_.height == clamped)
    return false;

  min_.height = clamped;
  max_.height = std::max(min_.height, max_.height);

  if (Geometry().height() < min_.height)
    InternalGeo().setHeight(min_.height);

  return true;
}

void Item::Damage()
{
  cscreen_->damageRegion(Geometry());
}

CompRect& Item::InternalGeo()
{
  return const_cast<CompRect&>(Geometry());
}

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

  ctx->vertexBuffer()->begin();
  ctx->glAddGeometry({texture_.quad.matrix}, texture_.quad.box, clip);

  if (ctx->vertexBuffer()->end())
    ctx->glDrawTexture(texture_, transformation, attrib, mask);
}

int TexturedItem::GetNaturalWidth() const
{
  return (texture_.st) ? texture_.st->width() : Item::GetNaturalWidth();
}

int TexturedItem::GetNaturalHeight() const
{
  return (texture_.st) ? texture_.st->height() : Item::GetNaturalHeight();
}

CompRect const& TexturedItem::Geometry() const
{
  return texture_.quad.box;
}

void TexturedItem::SetCoords(int x, int y)
{
  texture_.SetCoords(x, y);
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
  Relayout();
}

CompRect const& Layout::Geometry() const
{
  return rect_;
}

bool Layout::SetMaxWidth(int max_width)
{
  if (!Item::SetMaxWidth(max_width))
    return false;

  Relayout();
  return true;
}

bool Layout::SetMaxHeight(int max_height)
{
  if (!Item::SetMaxHeight(max_height))
    return false;

  Relayout();
  return true;
}

bool Layout::SetMinWidth(int min_width)
{
  if (!Item::SetMinWidth(min_width))
    return false;

  Relayout();
  return true;
}

bool Layout::SetMinHeight(int min_height)
{
  if (!Item::SetMinHeight(min_height))
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
    nux::Size content;

    for (auto const& item : items_)
    {
      if (!item->visible())
        continue;

      if (first_loop)
      {
        item->SetMinWidth(item->GetNaturalWidth());
        item->SetMaxWidth(max_.width);
        item->SetMinHeight(std::min(max_.height, item->GetNaturalHeight()));
        item->SetMaxHeight(max_.height);
      }

      auto const& item_geo = item->Geometry();
      content.height = std::max(content.height, item_geo.height());
      item->SetX(rect_.x() + content.width);

      if (item_geo.width() > 0)
        content.width += item_geo.width() + inner_padding_;
    }

    if (!items_.empty() && content.width > inner_padding_)
      content.width -= inner_padding_;

    if (content.width < min_.width)
      content.width = min_.width;

    if (content.height < min_.height)
      content.height = min_.height;

    int exceeding_width = content.width - max_.width;

    for (auto const& item : boost::adaptors::reverse(items_))
    {
      if (!item->visible())
        continue;

      auto const& item_geo = item->Geometry();

      if (exceeding_width > 0)
      {
        int old_width = item_geo.width();
        int max_item_width = std::max<int>(0, old_width - exceeding_width);
        item->SetMaxWidth(max_item_width);
        exceeding_width -= (old_width - max_item_width);
      }

      item->SetY(rect_.y() + (content.height - item_geo.height()) / 2);
    }

    rect_.setWidth(content.width);
    rect_.setHeight(content.height);

    first_loop = false;
  }
  while (rect_.width() > max_.width || rect_.height() > max_.height);
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
