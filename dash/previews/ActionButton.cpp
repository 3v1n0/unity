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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "unity-shared/DashStyle.h"
#include "ActionButton.h"
#include <NuxCore/Logger.h>

namespace
{
const int kMinButtonHeight = 30;
const int kMinButtonWidth  = 48;

nux::logging::Logger logger("unity.dash.actionbutton");
}

namespace unity
{
namespace dash
{

ActionButton::ActionButton(std::string const& label, NUX_FILE_LINE_DECL)
  : nux::Button(NUX_FILE_LINE_PARAM)
  , label_(label)
{
  SetLayoutPadding(2, 11, 2, 11);
  Button::SetLabel(label);

  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);

  Init();
}

ActionButton::~ActionButton()
{
}

void ActionButton::Init()
{
  InitTheme();

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

void ActionButton::InitTheme()
{
  if (!active_)
  {
    nux::Geometry const& geo = GetGeometry();

    prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
    active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    focus_.reset(new nux::CairoWrapper(geo, sigc::mem_fun(this, &ActionButton::RedrawFocusOverlay)));
  }

  SetMinimumHeight(kMinButtonHeight);
  SetMinimumWidth(kMinButtonWidth);
}

void ActionButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  int font_size = -1;
  Style::Instance().Button(cr, faked_state, label_, font_size, Alignment::CENTER, true);

  if (GetLabelFontSize() != font_size)
    SetLabelFontSize(font_size);
}

void ActionButton::RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr)
{
  Style::Instance().ButtonFocusOverlay(cr);
}

long ActionButton::ComputeContentSize()
{
  long ret = nux::Button::ComputeContentSize();

  nux::Geometry const& geo = GetGeometry();

  if (cached_geometry_ != geo)
  {
    if (prelight_) prelight_->Invalidate(geo);
    if (active_) active_->Invalidate(geo);
    if (normal_) normal_->Invalidate(geo);
    if (focus_) focus_->Invalidate(geo);

    cached_geometry_ = geo;
  }

  return ret;
}

void ActionButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
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

  if (IsMouseInside() || HasKeyboardFocus())
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
