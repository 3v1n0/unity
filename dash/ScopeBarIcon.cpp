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

#include "unity-shared/DashStyle.h"
#include "unity-shared/RawPixel.h"
#include "ScopeBarIcon.h"

#include "config.h"

namespace unity
{
namespace dash
{
namespace
{

RawPixel const FOCUS_OVERLAY_HEIGHT = 44_em;
RawPixel const FOCUS_OVERLAY_WIDTH  = 60_em;
RawPixel const TEXTURE_SIZE         = 24_em;
double const DEFAULT_SCALE          = 1.0;

}

NUX_IMPLEMENT_OBJECT_TYPE(ScopeBarIcon);

ScopeBarIcon::ScopeBarIcon(std::string id_, std::string icon_hint)
  : IconTexture(icon_hint, TEXTURE_SIZE)
  , id(id_)
  , active(false)
  , inactive_opacity_(0.4f)
  , scale_(DEFAULT_SCALE)
  , icon_hint_(icon_hint)
{
  SetMinMaxSize(FOCUS_OVERLAY_WIDTH.CP(scale_), FOCUS_OVERLAY_HEIGHT.CP(scale_));

  focus_layer_.reset(Style::Instance().FocusOverlay(FOCUS_OVERLAY_WIDTH.CP(scale_), FOCUS_OVERLAY_HEIGHT.CP(scale_)));

  SetOpacity(inactive_opacity_);

  SetAcceptKeyNavFocus(true);
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  active.changed.connect(sigc::mem_fun(this, &ScopeBarIcon::OnActiveChanged));
  key_nav_focus_change.connect([this](nux::Area*, bool, nux::KeyNavDirection){ QueueDraw(); });
}

ScopeBarIcon::~ScopeBarIcon()
{}

std::string ScopeBarIcon::IconHint() const
{
  return icon_hint_;
}

void ScopeBarIcon::UpdateScale(double scale)
{
  if (scale_ != scale)
  {
    scale_ = scale;
    int overlay_width  = FOCUS_OVERLAY_WIDTH.CP(scale_);
    int overlay_height = FOCUS_OVERLAY_HEIGHT.CP(scale_);

    SetMinMaxSize(overlay_width, overlay_height);
    focus_layer_.reset(Style::Instance().FocusOverlay(overlay_width, overlay_height));

    SetByIconName(icon_hint_, TEXTURE_SIZE.CP(scale_));
    ReLoadIcon();
  }
}

void ScopeBarIcon::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  graphics_engine.PushClippingRectangle(geo);

  if (HasKeyFocus() && focus_layer_)
  {
    nux::Geometry geo(GetGeometry());
    nux::AbstractPaintLayer* layer = focus_layer_.get();

    layer->SetGeometry(geo);
    layer->Renderlayer(graphics_engine);
  }

  if (texture())
  {
    unsigned int current_alpha_blend;
    unsigned int current_src_blend_factor;
    unsigned int current_dest_blend_factor;
    graphics_engine.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
    graphics_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    float opacity = active ? 1.0f : inactive_opacity_;
    int width = 0, height = 0;
    GetTextureSize(&width, &height);

    nux::Color col(1.0f * opacity, 1.0f * opacity, 1.0f * opacity, opacity);
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

    graphics_engine.QRP_1Tex(geo.x + ((geo.width - width) / 2),
                         geo.y + ((geo.height - height) / 2),
                         width,
                         height,
                         texture()->GetDeviceTexture(),
                         texxform,
                         col);

    graphics_engine.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  }

  graphics_engine.PopClippingRectangle();
}

void ScopeBarIcon::OnActiveChanged(bool is_active)
{
  QueueDraw();
}

// Introspectable
std::string ScopeBarIcon::GetName() const
{
  return "ScopeBarIcon";
}

void ScopeBarIcon::AddProperties(debug::IntrospectionData& wrapper)
{
  wrapper.add(GetAbsoluteGeometry());
  wrapper.add("name", id);
}

}
}
