/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "LensBarIcon.h"

#include "config.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(LensBarIcon);

LensBarIcon::LensBarIcon(std::string id_, std::string icon_hint)
  : IconTexture(icon_hint.c_str(), 24)
  , id(id_)
  , active(false)
  , inactive_opacity_(0.4f)
{
  SetMinimumWidth(24);
  SetMaximumWidth(24);
  SetMinimumHeight(24);
  SetMaximumHeight(24);
  SetOpacity(inactive_opacity_);

  active.changed.connect(sigc::mem_fun(this, &LensBarIcon::OnActiveChanged));
  mouse_enter.connect([&]
(int, int, unsigned long, unsigned long) { QueueDraw(); });
  mouse_leave.connect([&](int, int, unsigned long, unsigned long) { QueueDraw(); });
}

LensBarIcon::~LensBarIcon()
{}

void LensBarIcon::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  gfx_context.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(gfx_context, geo);

  if (!texture())
  {
    gfx_context.PopClippingRectangle();
    return;
  }

  float opacity = active || IsMouseInside() ? 1.0f : inactive_opacity_;
  int width = 0, height = 0;
  GetTextureSize(&width, &height);

  nux::Color col(1.0f * opacity, 1.0f * opacity, 1.0f * opacity, opacity);
  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

  gfx_context.QRP_1Tex(geo.x + ((geo.width - width) / 2),
                      geo.y + ((geo.height - height) / 2),
                      width,
                      height,
                      texture()->GetDeviceTexture(),
                      texxform,
                      col);

  gfx_context.PopClippingRectangle();
}

void LensBarIcon::OnActiveChanged(bool is_active)
{
  QueueDraw();
}

}
}
