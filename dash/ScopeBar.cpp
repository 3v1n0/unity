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

#include <glib/gstdio.h>
#include "ScopeBar.h"
#include <NuxCore/Logger.h>
#include "config.h"
#include <Nux/HLayout.h>
#include <Nux/LayeredLayout.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/GraphicsUtils.h"
#include "unity-shared/UBusMessages.h"

namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.scopebar");
namespace
{
// according to Q design the inner area of the scopebar should be 40px
// (without any borders)
const int SCOPEBAR_HEIGHT = 41;

}

NUX_IMPLEMENT_OBJECT_TYPE(ScopeBar);

ScopeBar::ScopeBar()
  : nux::View(NUX_TRACKER_LOCATION)
{
  SetupBackground();
  SetupLayout();
}

void ScopeBar::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_.reset(new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.2f), true, rop));
}

void ScopeBar::SetupLayout()
{
  layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout_->SetContentDistribution(nux::MAJOR_POSITION_CENTER);
  SetLayout(layout_);

  SetMinimumHeight(SCOPEBAR_HEIGHT);
  SetMaximumHeight(SCOPEBAR_HEIGHT);
}

void ScopeBar::AddScope(Scope::Ptr const& scope)
{
  ScopeBarIcon* icon = new ScopeBarIcon(scope->id, scope->icon_hint);

  icon->SetVisible(scope->visible);
  scope->visible.changed.connect([icon](bool visible) { icon->SetVisible(visible); } );
  icons_.push_back(icon);
  layout_->AddView(icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  AddChild(icon);

  icon->mouse_click.connect([&, icon] (int x, int y, unsigned long button, unsigned long keyboard) { SetActive(icon); });
  icon->key_nav_focus_activate.connect([&, icon](nux::Area*){ SetActive(icon); });
}

void ScopeBar::Activate(std::string id)
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

void ScopeBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  if (RedirectedAncestor())
    graphics::ClearGeometry(base);

  if (bg_layer_)
  {
    bg_layer_->SetGeometry(base);
    bg_layer_->Renderlayer(graphics_engine);
  }

  graphics_engine.PopClippingRectangle();
}

void ScopeBar::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  int pushed_paint_layers = 0;
  if (!IsFullRedraw())
  {
    if (RedirectedAncestor())
    {
      for (auto icon: icons_)
      {
        if (icon->IsVisible() && icon->IsRedrawNeeded())
          graphics::ClearGeometry(icon->GetGeometry());
      }
    }

    if (bg_layer_)
    {
      nux::GetPainter().PushLayer(graphics_engine, bg_layer_->GetGeometry(), bg_layer_.get());
      pushed_paint_layers++;
    }
  }
  else
  {
    nux::GetPainter().PushPaintLayerStack();
  }

  GetLayout()->ProcessDraw(graphics_engine, force_draw);

  if (IsFullRedraw())
  {
    nux::GetPainter().PopPaintLayerStack();
  }
  else if (pushed_paint_layers > 0)
  {
    nux::GetPainter().PopBackground(pushed_paint_layers);
  }

  for (auto icon: icons_)
  {
    if (icon->active && icon->IsVisible())
    {
      nux::Geometry const& geo = icon->GetGeometry();
      int middle = geo.x + geo.width/2;
      int size = 5;
      // Nux doesn't draw too well the small triangles, so let's draw a
      // bigger one and clip part of them using the "-1".
      int y = base.y - 1;

      nux::GetPainter().Draw2DTriangleColor(graphics_engine,
                                            middle - size, y,
                                            middle, y + size,
                                            middle + size, y,
                                            nux::color::White);

      break;
    }
  }

  graphics_engine.PopClippingRectangle();
}

void ScopeBar::SetActive(ScopeBarIcon* activated)
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
    scope_activated.emit(activated->id);
}

void ScopeBar::ActivateNext()
{
  bool activate_next = false;
  for (auto it = icons_.begin();
       it < icons_.end();
       it++)
  {
    ScopeBarIcon *icon = *it;

    if (activate_next && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_next = true;
  }

  // fallback. select first visible icon.
  for (auto it = icons_.begin(); it != icons_.end(); ++it)
  {
    ScopeBarIcon *icon = *it;
    if (icon->IsVisible())
    {
      SetActive(icon);
      break;
    }
  }
}

void ScopeBar::ActivatePrevious()
{
  bool activate_previous = false;
  for (auto rit = icons_.rbegin();
       rit < icons_.rend();
       ++rit)
  {
    ScopeBarIcon *icon = *rit;

    if (activate_previous && icon->IsVisible())
    {
      SetActive(icon);
      return;
    }
    if (icon->active)
      activate_previous = true;
  }

  // fallback. select last visible icon.
  for (auto rit = icons_.rbegin(); rit != icons_.rend(); ++rit)
  {
    ScopeBarIcon *icon = *rit;
    if (icon->IsVisible())
    {
      SetActive(icon);
      break;
    }
  }
}

// Keyboard navigation
bool ScopeBar::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string ScopeBar::GetName() const
{
  return "ScopeBar";
}

void ScopeBar::AddProperties(debug::IntrospectionData& wrapper)
{
  for (auto icon : icons_)
  {
    if (icon->active)
      wrapper.add("active-scope", icon->id());

    if (icon->HasKeyFocus())
      wrapper.add("focused-scope-icon", icon->id());
  }
}

std::string ScopeBar::GetActiveScopeId() const
{
  for (auto icon : icons_)
  {
    if (icon->active)
      return icon->id;
  }
  return "";
}

}
}
