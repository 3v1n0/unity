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

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <NuxGraphics/CairoGraphics.h>
#include <NuxGraphics/NuxGraphics.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/StaticCairoText.h"

#include "HudButton.h"
#include "HudPrivate.h"

namespace unity
{
namespace hud
{

namespace
{
const RawPixel HLAYOUT_LEFT_PADDING = 46_em;
const RawPixel HEIGHT = 42_em;
const char* const button_font = "Ubuntu 13"; // 17px = 13
}

NUX_IMPLEMENT_OBJECT_TYPE(HudButton);

HudButton::HudButton(NUX_FILE_LINE_DECL)
  : nux::Button(NUX_FILE_LINE_PARAM)
  , label([this] { return query_ ? query_->formatted_text : ""; })
  , is_rounded(false)
  , fake_focused(false)
  , scale(1.0)
  , is_focused_(false)
  , skip_draw_(true)
{
  hlayout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  hlayout_->SetLeftAndRightPadding(HLAYOUT_LEFT_PADDING.CP(scale), 0);
  SetLayout(hlayout_);

  InitTheme();

  key_nav_focus_change.connect([this](nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  });

  fake_focused.changed.connect([this](bool)
  {
    QueueDraw();
  });

  mouse_move.connect([this](int x, int y, int dx, int dy, unsigned int button, unsigned int key)
  {
    if (!fake_focused)
      fake_focused = true;
  });

  mouse_enter.connect([this](int x, int y, unsigned int button, unsigned int key)
  {
    fake_focused = true;
  });

  mouse_leave.connect([this](int x, int y, unsigned int button, unsigned int key)
  {
    fake_focused = false;
  });

  scale.changed.connect([this] (double scale) {
    hlayout_->SetLeftAndRightPadding(HLAYOUT_LEFT_PADDING.CP(scale), 0);
    InitTheme();
    SetQuery(query_);
  });
}

void HudButton::InitTheme()
{
  is_rounded.changed.connect([this](bool)
  {
    nux::Geometry const& geo = GetGeometry();
    prelight_->Invalidate(geo);
    active_->Invalidate(geo);
    normal_->Invalidate(geo);
  });

  SetMinimumHeight(HEIGHT.CP(scale));
  SetMaximumHeight(HEIGHT.CP(scale));

  nux::Geometry const& geo = GetGeometry();

  prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
  active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
  normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &HudButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
}

void HudButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  cairo_surface_set_device_scale(cairo_get_target(cr), scale, scale);
  dash::Style::Instance().SquareButton(cr, faked_state, "", is_rounded, 17, dash::Alignment::LEFT, true);
}

bool HudButton::AcceptKeyNavFocus()
{
  // The button will not receive the keyboard focus. The keyboard focus is always to remain with the
  // text entry in the hud.
  return false;
}

long HudButton::ComputeContentSize()
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
  if (skip_draw_)
    return;

  nux::Geometry const& geo = GetGeometry();
  GfxContext.PushClippingRectangle(geo);
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

  nux::Color col(nux::color::Black);
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
                      texture->GetWidth(),
                      texture->GetHeight(),
                      texture->GetDeviceTexture(),
                      texxform,
                      nux::color::White);

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  GfxContext.PopClippingRectangle();
}

void HudButton::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  if (skip_draw_)
    return;

  if (IsFullRedraw())
  {
    GfxContext.PushClippingRectangle(GetGeometry());
    hlayout_->ProcessDraw(GfxContext, force_draw);
    GfxContext.PopClippingRectangle();
  }
}

void HudButton::SetQuery(Query::Ptr const& query)
{
  query_ = query;

  if (!query_)
  {
    hlayout_->Clear();
    return;
  }

  auto items(impl::RefactorText(query_->formatted_text));

  hlayout_->Clear();
  for (auto item : items)
  {
    StaticCairoText* text = new StaticCairoText(item.first);
    text->SetScale(scale);
    text->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, item.second ? 1.0f : 0.5f));
    text->SetFont(button_font);
    text->SetInputEventSensitivity(false);
    hlayout_->AddView(text, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  }
}

Query::Ptr const& HudButton::GetQuery() const
{
  return query_;
}

void HudButton::SetSkipDraw(bool skip_draw)
{
  skip_draw_ = skip_draw;
}

// Introspectable
std::string HudButton::GetName() const
{
  return "HudButton";
}

void HudButton::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add("label", label())
    .add("focused", fake_focused());
}

} // namespace hud
} // namespace unity
