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

#include <NuxCore/Logger.h>

#include "CairoTexture.h"
#include "DashStyle.h"
#include "LensBar.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.lensbar");

const int FOCUS_OVERLAY_WIDTH = 56;
const int FOCUS_OVERLAY_HEIGHT = 40;

}

NUX_IMPLEMENT_OBJECT_TYPE(LensBar);

LensBar::LensBar()
  : nux::View(NUX_TRACKER_LOCATION)
{
  InitTheme();
  SetupBackground();
  SetupLayout();
  SetupHomeLens();
}

void LensBar::InitTheme()
{
  if (!focus_layer_)
  {
    focus_layer_.reset(Style::Instance().FocusOverlay(FOCUS_OVERLAY_WIDTH, FOCUS_OVERLAY_HEIGHT));
    over_layer_.reset(Style::Instance().FocusOverlay(FOCUS_OVERLAY_WIDTH, FOCUS_OVERLAY_HEIGHT));
  }
}

void LensBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_.reset(new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop));
}

void LensBar::SetupLayout()
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  layout_->SetSpaceBetweenChildren(40);
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

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); QueueDraw(); });
  icon->mouse_enter.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->mouse_leave.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->mouse_down.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->key_nav_focus_change.connect([&](nux::Area*, bool, nux::KeyNavDirection){ QueueDraw(); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
}

void LensBar::AddLens(Lens::Ptr& lens)
{
  LensBarIcon* icon = new LensBarIcon(lens->id, lens->icon_hint);
  icon->SetVisible(lens->visible);
  lens->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::eCenter, nux::eFix);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); QueueDraw(); });
  icon->mouse_enter.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->mouse_leave.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->mouse_down.connect([&] (int x, int y, unsigned long button, unsigned long keyboard) {  QueueDraw(); });
  icon->key_nav_focus_change.connect([&](nux::Area*, bool, nux::KeyNavDirection){ QueueDraw(); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
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

void LensBar::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  gfx_context.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(gfx_context, base);

  bg_layer_->SetGeometry(base);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, base, bg_layer_.get());

  for (auto icon : icons_)
  {
    if ((icon->HasKeyFocus() || icon->IsMouseInside()) && 
        focus_layer_ && over_layer_)
    {
      nux::Geometry geo(icon->GetGeometry());

      // Center it
      geo.x -= (FOCUS_OVERLAY_WIDTH - geo.width) / 2;
      geo.y -= (FOCUS_OVERLAY_HEIGHT - geo.height) / 2;
      geo.width = FOCUS_OVERLAY_WIDTH;
      geo.height = FOCUS_OVERLAY_HEIGHT;

      nux::AbstractPaintLayer* layer = icon->HasKeyFocus() ? focus_layer_.get() : over_layer_.get();

      layer->SetGeometry(geo);
      layer->Renderlayer(gfx_context);
    }
  }

  gfx_context.PopClippingRectangle();
}

void LensBar::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  gfx_context.PushClippingRectangle(GetGeometry());

  if (!IsFullRedraw())
    nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_.get());

  for (auto icon: icons_)
  {
    if ((icon->HasKeyFocus() || icon->IsMouseInside()) && !IsFullRedraw()
        && focus_layer_ && over_layer_)
    {
      nux::AbstractPaintLayer* layer = icon->HasKeyFocus() ? focus_layer_.get() : over_layer_.get();
      
      nux::GetPainter().PushLayer(gfx_context, focus_layer_->GetGeometry(), layer);
    }
  }

  layout_->ProcessDraw(gfx_context, force_draw);

  if (!IsFullRedraw())
    nux::GetPainter().PopBackground();

  gfx_context.PopClippingRectangle();
}

void LensBar::SetActive(LensBarIcon* activated)
{
  bool state_changed = false;

  for (auto icon: icons_)
  {
    bool state = icon == activated;

    if (icon->active != state)
      state_changed = true;

    icon->active = state;
  }

  if (state_changed)
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
std::string LensBar::GetName() const
{
  return "LensBar";
}

void LensBar::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);

  wrapper.add("focused-lens-icon", "");

  for( auto icon : icons_)
  {
    if (icon->active)
      wrapper.add("active-lens", icon->id.Get());

    if (icon->HasKeyFocus())
      wrapper.add("focused-lens-icon", icon->id.Get());
  }
}

}
}
