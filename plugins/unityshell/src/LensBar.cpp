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

#include "LensBar.h"

#include <NuxCore/Logger.h>

#include "config.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lensbar");
}

NUX_IMPLEMENT_OBJECT_TYPE(LensBar);

LensBar::LensBar()
  : nux::View(NUX_TRACKER_LOCATION)
{
  SetupBackground();

  layout_ = new nux::HLayout();
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  layout_->SetHorizontalInternalMargin(24);
  SetLayout(layout_);

  SetMinimumHeight(40);
  SetMaximumHeight(40);

  {
    std::string id = "home";
    IconTexture* icon = new IconTexture(PKGDATADIR"/lens-nav-home.svg", 26);
    icon->SetMinMaxSize(26, 26);
    icon->SetVisible(true);
    layout_->AddView(icon, 0, nux::eCenter, nux::eFix);

    icon->mouse_click.connect([&, id] (int x, int y, unsigned long button, unsigned long keyboard) { lens_activated.emit(id); });
  }
}

LensBar::~LensBar()
{}

void LensBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop);
}

void LensBar::AddLens(Lens::Ptr& lens)
{
  std::string id = lens->id;
  std::string icon_hint = lens->icon_hint;

  IconTexture* icon = new IconTexture(icon_hint.c_str(), 26);
  icon->SetMinMaxSize(26, 26);
  icon->SetVisible(lens->visible);
  lens->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  layout_->AddView(icon, 0, nux::eCenter, nux::eFix);

  icon->mouse_click.connect([&, id] (int x, int y, unsigned long button, unsigned long keyboard) { lens_activated.emit(id); });
}

long LensBar::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  return layout_->ProcessEvent(ievent, traverse_info, event_info);
}

void LensBar::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);

  bg_layer_->SetGeometry(geo);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, geo, bg_layer_);

  gfx_context.PopClippingRectangle();
}

void LensBar::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  gfx_context.PushClippingRectangle(GetGeometry());
  
  nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  
  layout_->ProcessDraw(gfx_context, force_draw);
  
  nux::GetPainter().PopBackground();
  
  gfx_context.PopClippingRectangle();
}

// Keyboard navigation
bool LensBar::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
const gchar* LensBar::GetName()
{
  return "LensBar";
}

void LensBar::AddProperties(GVariantBuilder* builder)
{}

}
}
