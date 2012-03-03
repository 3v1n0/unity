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
#include <UnityCore/Variant.h>

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
    , is_rounded(false)
    , is_focused_(false)
{
  Init();
}

HudButton::HudButton (const std::string label_, NUX_FILE_LINE_DECL)
    : nux::Button (NUX_FILE_LINE_PARAM)
    , is_rounded(false)
    , is_focused_(false)
{
  Init();
}

HudButton::HudButton (const std::string label_, nux::TextureArea *image, NUX_FILE_LINE_DECL)
    : nux::Button (image, NUX_FILE_LINE_PARAM)
    , is_rounded(false)
    , is_focused_(false)
{
  Init();
}

HudButton::HudButton (NUX_FILE_LINE_DECL)
    : nux::Button (NUX_FILE_LINE_PARAM)
    , is_rounded(false)
    , is_focused_(false)
{
  Init();
}

HudButton::~HudButton() {
}

void HudButton::Init()
{
  InitTheme();
  key_nav_focus_change.connect([this](nux::Area *area, bool recieving, nux::KeyNavDirection direction)
  {
    QueueDraw();
  });

  fake_focused.changed.connect([this](bool change)
  {
    QueueDraw();
  });
}

void HudButton::InitTheme()
{
  is_rounded.changed.connect([&] (bool rounded)
  {
    nux::Geometry geo = GetGeometry();
    prelight_->Invalidate(geo);
    active_->Invalidate(geo);
    normal_->Invalidate(geo);
  });

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
  dash::Style::Instance().SquareButton(cr, faked_state, label_,
                                           is_rounded, 17,
                                           dash::Alignment::LEFT, true);
}

bool HudButton::AcceptKeyNavFocus()
{
  // The button will not receive the keyboard focus. The keyboard focus is always to remain with the 
  // text entry in the hud.
  return false;
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
  texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  GfxContext.GetRenderStates().SetBlend(true);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x,
                       geo.y,
                       geo.width,
                       geo.height,
                       col);

  nux::BaseTexture* texture = normal_->GetTexture();
  if (HasKeyFocus() || fake_focused())
    texture = active_->GetTexture();
  else if (HasKeyFocus())
    texture = prelight_->GetTexture();
  else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
    texture = active_->GetTexture();

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      texture->GetWidth() + 1, // FIXME !! - jay, nux has gone crazy, unless i specify +1 here, it won't render the entire texture
                      texture->GetHeight(),
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

// Introspectable
std::string HudButton::GetName() const
{
  return "HudButton";
}

void HudButton::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add("label", label_);
}

}
}
