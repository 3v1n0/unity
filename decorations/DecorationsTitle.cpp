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

#include "DecorationsTitle.h"
#include "DecorationStyle.h"

namespace unity
{
namespace decoration
{

Title::Title()
{
  text.changed.connect(sigc::mem_fun(this, &Title::RebuildText));
  focused.changed.connect([this] (bool) { RebuildText(text()); });
}

void Title::RebuildText(std::string const& new_text)
{
  real_size_ = Style::Get()->TitleNaturalSize(new_text);

  auto state = focused() ? WidgetState::NORMAL : WidgetState::BACKDROP;
  cu::CairoContext text_ctx(real_size_.width, real_size_.height);
  Style::Get()->DrawTitle(new_text, state, text_ctx, real_size_.width, real_size_.height);
  SetTexture(text_ctx);
}

void Title::SetCoords(int x, int y)
{
  TexturedItem::SetCoords(x, y);
}

void Title::Draw(GLWindow* ctx, GLMatrix const& transformation, GLWindowPaintAttrib const& attrib,
                 CompRegion const& clip, unsigned mask)
{
  auto const& top = GetTopParent();
  float alignment = Style::Get()->TitleAlignmentValue();

  if (alignment > 0 && top)
  {
    auto const& top_geo = top->ContentGeometry();
    CompRect old_box(texture_.quad.box);
    int paint_x = std::max<int>(old_box.x(), top_geo.x() + (top_geo.width() - real_size_.width) * alignment);
    TexturedItem::SetCoords(paint_x, Geometry().y());
    texture_.quad.box.setGeometry(paint_x, old_box.y(), std::min(real_size_.width, old_box.width()), real_size_.height);
    TexturedItem::Draw(ctx, transformation, attrib, clip, mask);
    TexturedItem::SetCoords(old_box.x(), old_box.y());
  }
  else
  {
    TexturedItem::Draw(ctx, transformation, attrib, clip, mask);
  }
}

int Title::GetNaturalWidth() const
{
  return real_size_.width;
}

int Title::GetNaturalHeight() const
{
  return real_size_.height;
}

} // decoration namespace
} // unity namespace
