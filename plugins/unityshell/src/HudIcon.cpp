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
namespace
{
  nux::logging::Logger logger("unity.hud.icon");
}
  
namespace unity
{
namespace hud
{

Icon::Icon(nux::BaseTexture* texture, guint width, guint height)
  : unity::IconTexture(texture, width, height)
{
  Init();
  icon_renderer_.SetTargetSize(54, 46, 0);
}

Icon::Icon(const char* icon_name, unsigned int size, bool defer_icon_loading)
  : unity::IconTexture(icon_name, size, defer_icon_loading)
{
  Init();
}

Icon::~Icon()
{
}

void Icon::Init()
{
  SetMinimumWidth(66);
  SetMinimumHeight(66);
  background_ = nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_back_54.png", -1, true);
  gloss_ = nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_shine_54.png", -1, true);
  edge_ = nux::CreateTexture2DFromFile(PKGDATADIR"/launcher_icon_edge_54.png", -1,  true);
  
  texture_updated.connect([&] (nux::BaseTexture* texture) 
  {
    icon_texture_source_ = new HudIconTextureSource(nux::ObjectPtr<nux::BaseTexture>(texture));
    icon_texture_source_->ColorForIcon(_pixbuf_cached);
    QueueDraw();
    LOG_DEBUG(logger) << "got our texture";
  });
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
  icon_renderer_.SetTargetSize(54, 46, 0);
  icon_renderer_.PreprocessIcons(args, toplevel->GetGeometry());
  icon_renderer_.RenderIcon(GfxContext, arg, toplevel->GetGeometry(), toplevel->GetGeometry());
}


}
}

