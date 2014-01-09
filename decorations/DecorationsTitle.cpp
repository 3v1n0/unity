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

void Title::SetX(int x)
{
  float alignment = Style::Get()->TitleAlignmentValue();

  if (alignment > 0)
  {
    if (BasicContainer::Ptr const& top = GetTopParent())
    {
      auto const& top_geo = top->ContentGeometry();
      x = std::max<int>(x, top_geo.x() + (top_geo.width() - real_size_.width) * alignment);
    }
  }

  TexturedItem::SetX(x);
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
