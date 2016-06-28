/*
 * Copyright 2016 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *
 */

#include "LockScreenButton.h"

#include <Nux/HLayout.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/IconTexture.h"
#include "LockScreenSettings.h"

namespace unity
{
namespace lockscreen
{

namespace
{
const RawPixel HLAYOUT_RIGHT_PADDING = 10_em;
const int FONT_PX_SIZE = 17;
}

NUX_IMPLEMENT_OBJECT_TYPE(LockScreenButton);

LockScreenButton::LockScreenButton(std::string const& label, NUX_FILE_LINE_DECL)
  : nux::Button(NUX_FILE_LINE_PARAM)
  , scale(1.0)
  , label_(label)
{
  hlayout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  hlayout_->SetLeftAndRightPadding(0, HLAYOUT_RIGHT_PADDING.CP(scale));
  hlayout_->SetContentDistribution(nux::MAJOR_POSITION_END);
  SetLayout(hlayout_);

  activator_ = new IconTexture(dash::Style::Instance().GetLockScreenActivator(scale()));
  hlayout_->AddView(activator_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);

  InitTheme();

  scale.changed.connect([this] (double scale) {
    activator_->SetTexture(dash::Style::Instance().GetLockScreenActivator(scale));
    hlayout_->SetLeftAndRightPadding(0, HLAYOUT_RIGHT_PADDING.CP(scale));
    InitTheme();
  });

  key_down.connect([this] (unsigned long, unsigned long, unsigned long, const char*, unsigned short) {
    state_change.emit(this);
  });
}

void LockScreenButton::InitTheme()
{
  SetMinimumHeight(Settings::GRID_SIZE.CP(scale));
  SetMaximumHeight(Settings::GRID_SIZE.CP(scale));

  nux::Geometry const& geo = GetGeometry();
  normal_.reset(new nux::CairoWrapper(geo, sigc::mem_fun(this, &LockScreenButton::RedrawTheme)));
}

void LockScreenButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr)
{
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);
  dash::Style::Instance().LockScreenButton(cr, label_, FONT_PX_SIZE);
}

long LockScreenButton::ComputeContentSize()
{
  long ret = nux::Button::ComputeContentSize();
  nux::Geometry const& geo = GetGeometry();

  if (cached_geometry_ != geo)
  {
    normal_->Invalidate(geo);
    cached_geometry_ = geo;
  }

  return ret;
}

void LockScreenButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (IsFullRedraw())
  {
    GfxContext.PushClippingRectangle(GetGeometry());
    hlayout_->ProcessDraw(GfxContext, force_draw);
    GfxContext.PopClippingRectangle();
  }
}

void LockScreenButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  GfxContext.PushClippingRectangle(geo);
  gPainter.PaintBackground(GfxContext, geo);

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  GfxContext.GetRenderStates().SetBlend(true);

  nux::Color col(nux::color::Black);
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x, geo.y,
                       geo.width, geo.height,
                       col);

  nux::BaseTexture* texture = normal_->GetTexture();
  GfxContext.QRP_1Tex(geo.x, geo.y,
                      texture->GetWidth(), texture->GetHeight(),
                      texture->GetDeviceTexture(),
                      texxform, nux::color::White);

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);
  GfxContext.PopClippingRectangle();
}

bool LockScreenButton::InspectKeyEvent(unsigned int eventType, unsigned int key_sym, const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ENTER))
    return true;
  else
    return false;
}

} // namespace lockscreen
} // namespace unity