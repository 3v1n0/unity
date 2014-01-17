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
  text.changed.connect(sigc::mem_fun(this, &Title::OnTextChanged));
  focused.changed.connect([this] (bool) { if (texture_) RenderTexture(); });
  Style::Get()->title_font.changed.connect(sigc::mem_fun(this, &Title::OnFontChanged));
}

void Title::OnTextChanged(std::string const& new_text)
{
  bool damaged = false;
  auto real_size = Style::Get()->TitleNaturalSize(new_text);

  if (GetNaturalWidth() > real_size.width || GetNaturalHeight() > real_size.height)
  {
    damaged = true;
    Damage();
  }

  SetSize(real_size.width, real_size.height);
  texture_size_ = nux::Size();

  if (!damaged)
    Damage();
}

void Title::OnFontChanged(std::string const&)
{
  text.changed.emit(text());
}

void Title::RenderTexture()
{
  auto state = focused() ? WidgetState::NORMAL : WidgetState::BACKDROP;
  cu::CairoContext text_ctx(texture_size_.width, texture_size_.height);
  Style::Get()->DrawTitle(text().c_str(), state, text_ctx, texture_size_.width, texture_size_.height);
  texture_.SetTexture(text_ctx);
}

void Title::SetX(int x)
{
  float alignment = Style::Get()->TitleAlignmentValue();

  if (alignment > 0)
  {
    if (BasicContainer::Ptr const& top = GetTopParent())
    {
      auto const& top_geo = top->ContentGeometry();
      x = std::max<int>(x, top_geo.x() + (top_geo.width() - GetNaturalWidth()) * alignment);
    }
  }

  TexturedItem::SetX(x);
}

int Title::GetNaturalWidth() const
{
  return Item::GetNaturalWidth();
}

int Title::GetNaturalHeight() const
{
  return Item::GetNaturalHeight();
}

void Title::Draw(GLWindow* ctx, GLMatrix const& transformation, GLWindowPaintAttrib const& attrib,
                 CompRegion const& clip, unsigned mask)
{
  auto const& geo = Geometry();
  nux::Size tex_size(geo.width(), geo.height());

  if (texture_size_ != tex_size)
  {
    texture_size_ = tex_size;
    RenderTexture();
  }

  TexturedItem::Draw(ctx, transformation, attrib, clip, mask);
}

void Title::AddProperties(debug::IntrospectionData& data)
{
  TexturedItem::AddProperties(data);
  data.add("text", text())
  .add("texture_size", texture_size_);
}

} // decoration namespace
} // unity namespace
