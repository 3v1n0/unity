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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 */


#include "HudIcon.h"
#include "NuxCore/Logger.h"
#include "config.h"

namespace
{
  nux::logging::Logger logger("unity.hud.icon");
  const unsigned int tile_margin = 4;
  const unsigned int minimum_width = 64;
}

namespace unity
{
namespace hud
{

Icon::Icon()
  : IconTexture("", 0, true)
{
  background_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_back_54.png", -1, true));
  gloss_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_shine_54.png", -1, true));
  edge_.Adopt(nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_edge_54.png", -1,  true));

  texture_updated.connect([&] (nux::BaseTexture* texture)
  {
    icon_texture_source_ = new HudIconTextureSource(nux::ObjectPtr<nux::BaseTexture>(texture));
    icon_texture_source_->ColorForIcon(_pixbuf_cached);
    QueueDraw();
    LOG_DEBUG(logger) << "got our texture";
  });
}

void Icon::SetIcon(std::string const& icon_name, unsigned int icon_size, unsigned int tile_size)
{
  IconTexture::SetByIconName(icon_name, icon_size);
  icon_renderer_.SetTargetSize(tile_size, icon_size, 0);
  SetMinimumWidth(icon_size);
  SetMinimumHeight(icon_size);
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
  arg.render_center       = nux::Point3(32, 32, 0);
  arg.logical_center      = nux::Point3(52, 50, 0);
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
