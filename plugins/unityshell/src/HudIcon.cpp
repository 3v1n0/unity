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
//FIXME - we should be using the LauncherRenderer method of rendering icons here instead of compositing ourself
// to make sure we get the correct average background colour and running indicator
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
  icon_renderer_.SetTargetSize(52, 46, 0);
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
  });
}

void Icon::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  LOG_DEBUG(logger) << "attempting draw";
  if (texture() == nullptr)
    return;
  
  LOG_DEBUG(logger) << "doing draw";
  
  unity::ui::RenderArg arg;
  arg.icon = icon_texture_source_.GetPointer();
  arg.colorify            = nux::color::White;
  arg.running_arrow       = true;
  arg.running_on_viewport = true;
  arg.render_center       = nux::Point3(25, 25, 0);
  arg.logical_center      = nux::Point3(25, 25, 0);
  
  std::list<unity::ui::RenderArg> args;
  args.push_front(arg);
  
  icon_renderer_.PreprocessIcons(args, GetGeometry());
  
  icon_renderer_.RenderIcon(GfxContext, arg, GetGeometry(), GetGeometry());
  /*
  nux::Geometry geo = GetGeometry();
  // set up our texture mode
  nux::TexCoordXForm texxform;
  
  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      background_->GetWidth(),
                      background_->GetHeight(),
                      background_->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      gloss_->GetWidth(),
                      gloss_->GetHeight(),
                      gloss_->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      edge_->GetWidth(),
                      edge_->GetHeight(),
                      edge_->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  */
  unity::IconTexture::Draw(GfxContext, force_draw);
  
  
}


}
}

