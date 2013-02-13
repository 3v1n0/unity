/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */


#include "HudIcon.h"
#include "NuxCore/Logger.h"
#include "config.h"

namespace unity
{
namespace hud
{
DECLARE_LOGGER(logger, "unity.hud.icon");

Icon::Icon()
  : IconTexture("", 0, true)
{
  texture_updated.connect([this] (nux::ObjectPtr<nux::BaseTexture> const& texture)
  {
    icon_texture_source_ = new HudIconTextureSource(texture);
    icon_texture_source_->ColorForIcon(_pixbuf_cached);
    QueueDraw();
    LOG_DEBUG(logger) << "got our texture";
  });
}

void Icon::SetIcon(std::string const& icon_name, unsigned int icon_size, unsigned int tile_size, unsigned int padding)
{
  IconTexture::SetByIconName(icon_name, icon_size);
  icon_renderer_.SetTargetSize(tile_size, icon_size, 0);

  SetMinimumHeight(tile_size + padding);
  SetMinimumWidth(tile_size + padding);
}

void Icon::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (texture() == nullptr)
    return;

  unity::ui::RenderArg arg;
  arg.icon = icon_texture_source_.GetPointer();
  arg.colorify            = nux::color::White;
  arg.running_arrow       = true;
  arg.running_on_viewport = true;
  arg.render_center       = nux::Point3(GetMinimumWidth() / 2.0f, GetMinimumHeight() / 2.0f, 0.0f);
  arg.logical_center      = arg.render_center;
  arg.window_indicators   = true;
  arg.backlight_intensity = 1.0f;
  arg.alpha               = 1.0f;

  std::list<unity::ui::RenderArg> args;
  args.push_front(arg);

  auto toplevel = GetToplevel();
  icon_renderer_.PreprocessIcons(args, toplevel->GetGeometry());
  icon_renderer_.RenderIcon(GfxContext, arg, toplevel->GetGeometry(), toplevel->GetGeometry());
}

std::string Icon::GetName() const
{
  return "HudEmbeddedIcon";
}

}
}
