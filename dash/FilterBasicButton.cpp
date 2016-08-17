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

#include "unity-shared/DashStyle.h"
#include "unity-shared/UnitySettings.h"
#include "FilterBasicButton.h"

namespace unity
{
namespace dash
{
namespace
{
const RawPixel BUTTON_HEIGHT = 30_em;
const RawPixel MIN_BUTTON_WIDTH  = 48_em;
const int FONT_SIZE_PX = 15; // 15px == 11pt
}

NUX_IMPLEMENT_OBJECT_TYPE(FilterBasicButton);

FilterBasicButton::FilterBasicButton(nux::TextureArea* image, NUX_FILE_LINE_DECL)
  : FilterBasicButton(std::string(), image, NUX_FILE_LINE_PARAM)
{}

FilterBasicButton::FilterBasicButton(NUX_FILE_LINE_DECL)
  : FilterBasicButton(std::string(), NUX_FILE_LINE_PARAM)
{}

FilterBasicButton::FilterBasicButton(std::string const& label, NUX_FILE_LINE_DECL)
  : FilterBasicButton(label, nullptr, NUX_FILE_LINE_PARAM)
{}

FilterBasicButton::FilterBasicButton(std::string const& label, nux::TextureArea* image, NUX_FILE_LINE_DECL)
  : nux::ToggleButton(image, NUX_FILE_LINE_PARAM)
  , scale(1.0)
  , label_(label)
{
  InitTheme();
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  clear_before_draw_ = true;

  key_nav_focus_change.connect([this] (nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  });

  key_nav_focus_activate.connect([this](nux::Area*)
  {
    if (GetInputEventSensitivity())
      Active() ? Deactivate() : Activate();
  });

  scale.changed.connect(sigc::mem_fun(this, &FilterBasicButton::UpdateScale));
  Settings::Instance().font_scaling.changed.connect(sigc::hide(sigc::mem_fun(this, &FilterBasicButton::InitTheme)));
  Style::Instance().changed.connect(sigc::mem_fun(this, &FilterBasicButton::InitTheme));
}

void FilterBasicButton::InitTheme()
{
  nux::Geometry const& geo = GetGeometry();

  prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
  active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
  normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &FilterBasicButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
  focus_.reset(new nux::CairoWrapper(geo, sigc::mem_fun(this, &FilterBasicButton::RedrawFocusOverlay)));

  double font_scaling = Settings::Instance().font_scaling() * scale;
  SetMinimumWidth(MIN_BUTTON_WIDTH.CP(font_scaling));
  ApplyMinWidth();

  SetMinimumHeight(BUTTON_HEIGHT.CP(font_scaling));
  SetMaximumHeight(BUTTON_HEIGHT.CP(font_scaling));
}

void FilterBasicButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);
  Style::Instance().Button(cr, faked_state, label_, FONT_SIZE_PX, Alignment::CENTER, true);
}

void FilterBasicButton::RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr)
{
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);
  Style::Instance().ButtonFocusOverlay(cr);
}

void FilterBasicButton::UpdateScale(double scale)
{
  InitTheme();
  QueueDraw();
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

void FilterBasicButton::SetClearBeforeDraw(bool clear_before_draw)
{
  clear_before_draw_ = clear_before_draw;
}

void FilterBasicButton::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  graphics_engine.PushClippingRectangle(geo);

  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;
  graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
  graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  graphics_engine.QRP_Color(geo.x,
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

  graphics_engine.QRP_1Tex(geo.x,
                      geo.y,
                      geo.width,
                      geo.height,
                      texture->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  if (HasKeyboardFocus())
  {
    graphics_engine.QRP_1Tex(geo.x,
                        geo.y,
                        geo.width,
                        geo.height,
                        focus_->GetTexture()->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);

  graphics_engine.PopClippingRectangle();
}

std::string const& FilterBasicButton::GetLabel() const
{
  return label_;
}

} // namespace dash
} // namespace unity
