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

#include "DashStyle.h"
#include "FilterBasicButton.h"

namespace
{
const int kMinButtonHeight = 30;
const int kMinButtonWidth  = 48;
}

namespace unity
{
namespace dash
{

FilterBasicButton::FilterBasicButton(nux::TextureArea* image, NUX_FILE_LINE_DECL)
  : nux::ToggleButton(image, NUX_FILE_LINE_PARAM)
{
  Init();
}

FilterBasicButton::FilterBasicButton(std::string const& label, NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
  , label_(label)
{
  Init();
}

FilterBasicButton::FilterBasicButton(std::string const& label, nux::TextureArea* image, NUX_FILE_LINE_DECL)
  : nux::ToggleButton(image, NUX_FILE_LINE_PARAM)
  , label_(label)
{
  Init();
}

FilterBasicButton::FilterBasicButton(NUX_FILE_LINE_DECL)
  : nux::ToggleButton(NUX_FILE_LINE_PARAM)
{
  Init();
}

FilterBasicButton::~FilterBasicButton()
{
}

void FilterBasicButton::Init()
{

  InitTheme();
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  key_nav_focus_change.connect([&] (nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  });

  key_nav_focus_activate.connect([&](nux::Area*)
  {
    if (GetInputEventSensitivity())
      Active() ? Deactivate() : Activate();
  });
}

void FilterBasicButton::InitTheme()
{
  if (!active_)
  {
    nux::Geometry const& geo = GetGeometry();

    prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
    active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    focus_.reset(new nux::CairoWrapper(geo, sigc::mem_fun(this, &FilterBasicButton::RedrawFocusOverlay)));
  }

  SetMinimumHeight(kMinButtonHeight);
  SetMinimumWidth(kMinButtonWidth);
}

void FilterBasicButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  Style::Instance().Button(cr, faked_state, label_, -1, Alignment::CENTER, true);
}

void FilterBasicButton::RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr)
{
  Style::Instance().ButtonFocusOverlay(cr);
}

long FilterBasicButton::ComputeContentSize()
{
  long ret = nux::Button::ComputeContentSize();

  nux::Geometry const& geo = GetGeometry();

  if (cached_geometry_ != geo)
  {
    prelight_->Invalidate(geo);
    active_->Invalidate(geo);
    normal_->Invalidate(geo);
    focus_->Invalidate(geo);

    cached_geometry_ = geo;
  }

  return ret;
}

void FilterBasicButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
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
  if (Active())
    texture = active_->GetTexture();
  else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
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

  if (HasKeyboardFocus())
  {
    GfxContext.QRP_1Tex(geo.x,
                        geo.y,
                        geo.width,
                        geo.height,
                        focus_->GetTexture()->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
}

} // namespace dash
} // namespace unity
