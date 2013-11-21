// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#include "SearchBarSpinner.h"

#include <Nux/VLayout.h>

#include "unity-shared/DashStyle.h"

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(SearchBarSpinner);

SearchBarSpinner::SearchBarSpinner()
  : nux::View(NUX_TRACKER_LOCATION),
    state_(STATE_READY),
    search_timeout_(-1),
    rotation_(0.0f)
{
  dash::Style& style = dash::Style::Instance();

  magnify_ = style.GetSearchMagnifyIcon();
  circle_ = style.GetSearchCircleIcon();
  close_ = style.GetSearchCloseIcon();
  spin_ = style.GetSearchSpinIcon();

  rotate_.Identity();
  rotate_.Rotate_z(0.0);
}

void
SearchBarSpinner::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  nux::TexCoordXForm texxform;

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

  unsigned int current_alpha_blend;
  unsigned int current_src_blend_factor;
  unsigned int current_dest_blend_factor;
  GfxContext.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  GfxContext.GetRenderStates().SetBlend(true,  GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (state_ == STATE_READY)
  {
    GfxContext.QRP_1Tex(geo.x + ((geo.width - magnify_->GetWidth()) / 2),
                        geo.y + ((geo.height - magnify_->GetHeight()) / 2),
                        magnify_->GetWidth(),
                        magnify_->GetHeight(),
                        magnify_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  else if (state_ == STATE_SEARCHING)
  {
    nux::Geometry spin_geo(geo.x + ((geo.width - spin_->GetWidth()) / 2),
                           geo.y + ((geo.height - spin_->GetHeight()) / 2),
                           spin_->GetWidth(),
                           spin_->GetHeight());
    // Geometry (== Rect) uses integers which were rounded above,
    // hence an extra 0.5 offset for odd sizes is needed
    // because pure floating point is not being used.
    int spin_offset_w = !(geo.width % 2) ? 0 : 1;
    int spin_offset_h = !(geo.height % 2) ? 0 : 1;

    nux::Matrix4 matrix_texture;
    matrix_texture = nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                          -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;
    matrix_texture = rotate_ * matrix_texture;
    matrix_texture = nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                             spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0) * matrix_texture;

    GfxContext.SetModelViewMatrix(GfxContext.GetModelViewMatrix() * matrix_texture);

    GfxContext.QRP_1Tex(spin_geo.x,
                        spin_geo.y,
                        spin_geo.width,
                        spin_geo.height,
                        spin_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    // revert to model view matrix stack
    GfxContext.ApplyModelViewMatrix();
  }
  else
  {
    GfxContext.QRP_1Tex(geo.x + ((geo.width - circle_->GetWidth()) / 2),
                        geo.y + ((geo.height - circle_->GetHeight()) / 2),
                        circle_->GetWidth(),
                        circle_->GetHeight(),
                        circle_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    GfxContext.QRP_1Tex(geo.x + ((geo.width - close_->GetWidth()) / 2),
                        geo.y + ((geo.height - close_->GetHeight()) / 2),
                        close_->GetWidth(),
                        close_->GetHeight(),
                        close_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  GfxContext.PopClippingRectangle();

  GfxContext.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);

  if (state_ == STATE_SEARCHING && !frame_timeout_)
  {
    frame_timeout_.reset(new glib::Timeout(22, sigc::mem_fun(this, &SearchBarSpinner::OnFrameTimeout)));
  }
}

void
SearchBarSpinner::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

bool
SearchBarSpinner::OnFrameTimeout()
{
  rotation_ += 0.1f;

  if (rotation_ >= 360.0f)
    rotation_ = 0.0f;

  rotate_.Rotate_z(rotation_);
  QueueDraw();

  frame_timeout_.reset();
  return false;
}

void
SearchBarSpinner::SetSpinnerTimeout(int timeout)
{
  search_timeout_ = timeout;
}

void
SearchBarSpinner::SetState(SpinnerState state)
{
  if (state_ == state)
    return;

  state_ = state;
  spinner_timeout_.reset();
  rotate_.Rotate_z(0.0f);
  rotation_ = 0.0f;

  if (search_timeout_ > 0 && state_== STATE_SEARCHING)
  {
    spinner_timeout_.reset(new glib::Timeout(search_timeout_, [this] {
      state_ = STATE_READY;
      return false;
    }));
  }

  QueueDraw();
}

SpinnerState
SearchBarSpinner::GetState() const
{
  return state_;
}

std::string
SearchBarSpinner::GetName() const
{
  return "SearchBarSpinner";
}

void SearchBarSpinner::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add(GetAbsoluteGeometry());
}

//
// Key navigation
//
bool
SearchBarSpinner::AcceptKeyNavFocus()
{
  return false;
}

}
