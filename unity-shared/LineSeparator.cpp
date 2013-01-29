/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#include "LineSeparator.h"

#include "Nux/Nux.h"

namespace unity
{

HSeparator::HSeparator()
{
  SetMinimumHeight(1);
  SetMaximumHeight(1);
}

HSeparator::HSeparator(nux::Color const& color, float alpha0, float alpha1, int border)
  : AbstractSeparator(color, alpha0, alpha1, border)
{
  SetMinimumHeight(1);
  SetMaximumHeight(1);
}

HSeparator::~HSeparator()
{
}

void HSeparator::Draw(nux::GraphicsEngine &GfxContext, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  int y0 = base.y + base.GetHeight() / 2;

  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  nux::GetGraphicsDisplay()->GetGraphicsEngine()->GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (base.GetWidth() - 2 * border_size_ > 0)
  {
    nux::Color color0 = color_ * alpha0_;
    nux::Color color1 = color_ * alpha1_;
    nux::GetPainter().Draw2DLine(GfxContext, base.x, y0, base.x + border_size_, y0, color0, color1);
    nux::GetPainter().Draw2DLine(GfxContext, base.x + border_size_, y0, base.x + base.GetWidth() - border_size_, y0, color1, color1);
    nux::GetPainter().Draw2DLine(GfxContext, base.x + base.GetWidth() - border_size_, y0, base.x + base.GetWidth(), y0, color1, color0);
  }
  else
  {
    nux::Color color1 = color_ *  alpha1_;
    nux::GetPainter().Draw2DLine(GfxContext, base.x, y0, base.x + base.GetWidth(), y0, color1, color1);
  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
}

} // namespace unity
