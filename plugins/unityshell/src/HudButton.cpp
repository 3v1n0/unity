/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */
#include "config.h"

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/NuxGraphics.h>
#include <UnityCore/GLibWrapper.h>
#include "DashStyle.h"

#include "HudButton.h"

namespace
{
nux::logging::Logger logger("unity.hud.HudButton");
}

namespace unity 
{
namespace hud 
{


HudButton::HudButton (nux::TextureArea *image, NUX_FILE_LINE_DECL)
    : nux::Button (image, NUX_FILE_LINE_PARAM)
    , is_focused_(false)
{
  InitTheme();
  OnKeyNavFocusChange.connect([this](nux::Area *area){ QueueDraw(); });
}

HudButton::HudButton (const std::string label_, NUX_FILE_LINE_DECL)
    : nux::Button (NUX_FILE_LINE_PARAM)
    , is_focused_(false)
{
  InitTheme();
}

HudButton::HudButton (const std::string label_, nux::TextureArea *image, NUX_FILE_LINE_DECL)
    : nux::Button (image, NUX_FILE_LINE_PARAM)
    , is_focused_(false)
{
  InitTheme();
}

HudButton::HudButton (NUX_FILE_LINE_DECL)
    : nux::Button (NUX_FILE_LINE_PARAM)
    , is_focused_(false)
{
  InitTheme();
}

HudButton::~HudButton() {
}

void HudButton::InitTheme()
{
  SetMinimumHeight(42);
  if (!active_)
  {
    nux::Geometry const& geo = GetGeometry();

    prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
    active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
  }
}

void HudButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  dash::Style::Instance().Button(cr, faked_state, label_, dash::Alignment::LEFT);
}

bool HudButton::AcceptKeyNavFocus()
{
  return true;
}


long HudButton::ComputeContentSize ()
{
  long ret = nux::Button::ComputeContentSize();
  nux::Geometry const& geo = GetGeometry();

  if (cached_geometry_ != geo)
  {
    prelight_->Invalidate(geo);
    active_->Invalidate(geo);
    normal_->Invalidate(geo);

    cached_geometry_ = geo;
  }
  
  return ret;
} 

void HudButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) 
{
  nux::Geometry const& geo = GetGeometry();

  gPainter.PaintBackground(GfxContext, geo);
  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x,
                       geo.y,
                       geo.width,
                       geo.height,
                       col);

  nux::BaseTexture* texture = normal_->GetTexture();
  if (HasKeyFocus())
    texture = active_->GetTexture();
  else if (HasKeyFocus())
    texture = prelight_->GetTexture();
  else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
    texture = active_->GetTexture();

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      geo.width,
                      geo.height,
                      texture->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
}

void HudButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
}

void HudButton::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
  nux::Button::PostDraw(GfxContext, force_draw);
}

void HudButton::SetQuery(Query::Ptr query)
{
  query_ = query;
  label_ = query->formatted_text;
  label = query->formatted_text;
}

Query::Ptr HudButton::GetQuery()
{
  return query_;
}
}
}
