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
  SetupLayout();
  SetupHomeLens();
}

LensBar::~LensBar()
{
  delete bg_layer_;
}

void LensBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop);
}

void LensBar::SetupLayout()
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  layout_->SetHorizontalInternalMargin(40);
  SetLayout(layout_);

  SetMinimumHeight(46);
  SetMaximumHeight(46);
}

void LensBar::SetupHomeLens()
{
  LensBarIcon* icon = new LensBarIcon("home.lens", PKGDATADIR"/lens-nav-home.svg");
  icon->SetVisible(true);
  icon->active = true;
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::eCenter, nux::MINOR_SIZE_FULL);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
}

void LensBar::AddLens(Lens::Ptr& lens)
{
  LensBarIcon* icon = new LensBarIcon(lens->id, lens->icon_hint);
  icon->SetVisible(lens->visible);
  lens->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::eCenter, nux::eFix);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
}

void LensBar::Activate(std::string id)
{
  for (auto icon: icons_)
  {
    if (icon->id == id)
    {
      SetActive(icon);
      break;
    }
  }
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
  
  if (!IsFullRedraw())
    nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  
  layout_->ProcessDraw(gfx_context, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

  for (auto icon: icons_)
    if (icon->active)
	{
      nux::Geometry geo = icon->GetGeometry();
      int middle = geo.x + geo.width/2;
      int size = 5;
      int y = geo.y - 11;

      nux::GetPainter().Draw2DTriangleColor(gfx_context,
                                            middle - size, y,
                                            middle, y + size,
                                            middle + size, y,
                                            nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

	}

  gfx_context.PopClippingRectangle();
}

void LensBar::SetActive(LensBarIcon* activated)
{
  for (auto icon: icons_)
    icon->active = icon == activated;

  lens_activated.emit(activated->id);
}

void LensBar::ActivateNext()
{
  bool activate_next = false; 
  for (auto it = icons_.begin();
       it < icons_.end();
       it++)
  {
    LensBarIcon *icon = *it;
    
    if (activate_next && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_next = true;
  }
  SetActive(icons_[0]);

}

void LensBar::ActivatePrevious()
{
  bool activate_previous = false;
  
  for (auto it = icons_.rbegin();
       it < icons_.rend();
       ++it)
  {
    LensBarIcon *icon = *it;
    
    if (activate_previous && icon->IsVisible())
    {
	SetActive(icon);
	return;
    }
    if (icon->active)
      activate_previous = true;
  }
  SetActive(icons_.back());
  
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
