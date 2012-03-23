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
#include <UnityCore/Variant.h>

#include "DashStyle.h"

namespace unity
{

NUX_IMPLEMENT_OBJECT_TYPE(SearchBarSpinner);

SearchBarSpinner::SearchBarSpinner()
  : nux::View(NUX_TRACKER_LOCATION),
    _state(STATE_READY),
    _rotation(0.0f),
    _spinner_timeout(0),
    _frame_timeout(0)
{
  dash::Style& style = dash::Style::Instance();

  _magnify = style.GetSearchMagnifyIcon();
  _circle = style.GetSearchCircleIcon();
  _close = style.GetSearchCloseIcon();
  _spin = style.GetSearchSpinIcon();

  _2d_rotate.Identity();
  _2d_rotate.Rotate_z(0.0);
}

SearchBarSpinner::~SearchBarSpinner()
{
  if (_spinner_timeout)
    g_source_remove(_spinner_timeout);

  if (_frame_timeout)
    g_source_remove(_frame_timeout);
}

void
SearchBarSpinner::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();
  nux::TexCoordXForm texxform;

  GfxContext.PushClippingRectangle(geo);

  nux::GetPainter().PaintBackground(GfxContext, geo);

  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.min_filter = nux::TEXFILTER_LINEAR;
  texxform.mag_filter = nux::TEXFILTER_LINEAR;

  if (_state == STATE_READY)
  {
    GfxContext.QRP_1Tex(geo.x + ((geo.width - _magnify->GetWidth()) / 2),
                        geo.y + ((geo.height - _magnify->GetHeight()) / 2),
                        _magnify->GetWidth(),
                        _magnify->GetHeight(),
                        _magnify->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  else if (_state == STATE_SEARCHING)
  {
    nux::Geometry spin_geo(geo.x + ((geo.width - _spin->GetWidth()) / 2),
                           geo.y + ((geo.height - _spin->GetHeight()) / 2),
                           _spin->GetWidth(),
                           _spin->GetHeight());
    // Geometry (== Rect) uses integers which were rounded above,
    // hence an extra 0.5 offset for odd sizes is needed
    // because pure floating point is not being used.
    int spin_offset_w = !(geo.width % 2) ? 0 : 1;
    int spin_offset_h = !(geo.height % 2) ? 0 : 1;

    GfxContext.PushModelViewMatrix(nux::Matrix4::TRANSLATE(-spin_geo.x - (spin_geo.width + spin_offset_w) / 2.0f,
                                                           -spin_geo.y - (spin_geo.height + spin_offset_h) / 2.0f, 0));
    GfxContext.PushModelViewMatrix(_2d_rotate);
    GfxContext.PushModelViewMatrix(nux::Matrix4::TRANSLATE(spin_geo.x + (spin_geo.width + spin_offset_w) / 2.0f,
                                                           spin_geo.y + (spin_geo.height + spin_offset_h) / 2.0f, 0));

    GfxContext.QRP_1Tex(spin_geo.x,
                        spin_geo.y,
                        spin_geo.width,
                        spin_geo.height,
                        _spin->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    GfxContext.PopModelViewMatrix();
    GfxContext.PopModelViewMatrix();
    GfxContext.PopModelViewMatrix();
  }
  else
  {
    GfxContext.QRP_1Tex(geo.x + ((geo.width - _circle->GetWidth()) / 2),
                        geo.y + ((geo.height - _circle->GetHeight()) / 2),
                        _circle->GetWidth(),
                        _circle->GetHeight(),
                        _circle->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    GfxContext.QRP_1Tex(geo.x + ((geo.width - _close->GetWidth()) / 2),
                        geo.y + ((geo.height - _close->GetHeight()) / 2),
                        _close->GetWidth(),
                        _close->GetHeight(),
                        _close->GetDeviceTexture(),
                        texxform,
                        nux::color::White);
  }
  GfxContext.PopClippingRectangle();

  if (_state == STATE_SEARCHING && !_frame_timeout)
    _frame_timeout = g_timeout_add(22, (GSourceFunc)SearchBarSpinner::OnFrame, this);
}

void
SearchBarSpinner::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

gboolean
SearchBarSpinner::OnTimeout(SearchBarSpinner* self)
{
  self->_state = STATE_READY;
  return FALSE;
}

gboolean
SearchBarSpinner::OnFrame(SearchBarSpinner* self)
{
  self->_rotation += 0.1f;
  if (self->_rotation >= 360.0f)
    self->_rotation = 0.0f;

  self->_2d_rotate.Rotate_z(self->_rotation);
  self->_frame_timeout = 0;

  self->QueueDraw();
  return FALSE;
}

void
SearchBarSpinner::SetState(SpinnerState state)
{
  if (_state == state)
    return;

  _state = state;

  if (_spinner_timeout)
    g_source_remove(_spinner_timeout);
  _spinner_timeout = 0;

  _2d_rotate.Rotate_z(0.0f);
  _rotation = 0.0f;

  if (_state == STATE_SEARCHING)
  {
    _spinner_timeout = g_timeout_add_seconds(5, (GSourceFunc)SearchBarSpinner::OnTimeout, this);
  }

  QueueDraw();
}

std::string
SearchBarSpinner::GetName() const
{
  return "SearchBarSpinner";
}

void SearchBarSpinner::AddProperties(GVariantBuilder* builder)
{
  nux::Geometry geo = GetGeometry();

  variant::BuilderWrapper(builder)
    .add("x", geo.x)
    .add("y", geo.y)
    .add("width", geo.width)
    .add("height", geo.height);
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
