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
#include "unity-shared/RawPixel.h"

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(SearchBarSpinner);

SearchBarSpinner::SearchBarSpinner()
  : nux::View(NUX_TRACKER_LOCATION)
  , scale(1.0)
  , state_(STATE_READY)
  , search_timeout_(-1)
  , rotation_(0.0f)
{
  rotate_.Identity();
  rotate_.Rotate_z(0.0);
  UpdateScale(scale);

  scale.changed.connect(sigc::mem_fun(this, &SearchBarSpinner::UpdateScale));
}

void SearchBarSpinner::UpdateScale(double scale)
{
  auto& style = dash::Style::Instance();

  magnify_ = style.GetSearchMagnifyIcon(scale);
  circle_ = style.GetSearchCircleIcon(scale);
  close_ = style.GetSearchCloseIcon(scale);
  spin_ = style.GetSearchSpinIcon(scale);

  SetMinMaxSize(magnify_->GetWidth(), magnify_->GetHeight());
  QueueDraw();
}

void SearchBarSpinner::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();
  nux::TexCoordXForm texxform;

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
  texxform.SetFilter(nux::TEXFILTER_LINEAR, nux::TEXFILTER_LINEAR);

  unsigned int current_alpha_blend;
  unsigned int current_src_blend_factor;
  unsigned int current_dest_blend_factor;
  GfxContext.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  GfxContext.GetRenderStates().SetBlend(true,  GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  if (state_ == STATE_READY)
  {
    nux::Size magnifier_size(magnify_->GetWidth(), magnify_->GetHeight());

    GfxContext.QRP_1Tex(geo.x + ((geo.width - magnifier_size.width) / 2),
                        geo.y + ((geo.height - magnifier_size.height) / 2),
                        magnifier_size.width,
                        magnifier_size.height,
                        magnify_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  else if (state_ == STATE_SEARCHING)
  {
    nux::Size spin_size(spin_->GetWidth(), spin_->GetHeight());
    nux::Geometry spin_geo(geo.x + ((geo.width - spin_size.width) / 2),
                           geo.y + ((geo.height - spin_size.height) / 2),
                           spin_size.width,
                           spin_size.height);
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
    nux::Size circle_size(circle_->GetWidth(), circle_->GetHeight());
    GfxContext.QRP_1Tex(geo.x + ((geo.width - circle_size.width) / 2),
                        geo.y + ((geo.height - circle_size.height) / 2),
                        circle_size.width,
                        circle_size.height,
                        circle_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    nux::Size close_size(close_->GetWidth(), close_->GetHeight());
    GfxContext.QRP_1Tex(geo.x + ((geo.width - close_size.width) / 2),
                        geo.y + ((geo.height - close_size.height) / 2),
                        close_size.width,
                        close_size.height,
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
