// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <Nux/Nux.h>

#include "PlacesOverlayVScrollBar.h"
#include "CairoTexture.h"

namespace
{
  int const PROXIMITY = 7;
  int const SCROLL_ANIMATION = 400;
  int const MAX_CONNECTOR_ANIMATION = 200;
}

namespace unity
{
namespace dash
{

PlacesOverlayVScrollBar::PlacesOverlayVScrollBar(NUX_FILE_LINE_DECL)
  : PlacesVScrollBar(NUX_FILE_LINE_PARAM)
  , overlay_window_(new VScrollBarOverlayWindow(_track->GetAbsoluteGeometry()))
  , area_prox_(this, PROXIMITY)
  , thumb_above_slider_(false)
  , connector_height_(0)
  , mouse_down_offset_(0)
  , delta_update_(0)
{
  area_prox_.mouse_near.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseNear));
  area_prox_.mouse_beyond.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseBeyond));

  overlay_window_->mouse_enter.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseEnter));
  overlay_window_->mouse_leave.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseLeave));
  overlay_window_->mouse_down.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseDown));
  overlay_window_->mouse_up.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseUp));
  overlay_window_->mouse_click.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseClick));
  overlay_window_->mouse_move.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseMove));
  overlay_window_->mouse_drag.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseDrag));

  _track->geometry_changed.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnTrackGeometryChanged));
  OnVisibleChanged.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnVisibilityChanged));
  OnSensitiveChanged.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnSensitivityChanged));
}

void PlacesOverlayVScrollBar::OnTrackGeometryChanged(nux::Area* /*area*/, nux::Geometry& /*geo*/)
{
  UpdateStepY();
  overlay_window_->UpdateGeometry(_track->GetAbsoluteGeometry());

  if (overlay_window_->IsVisible() && !IsScrollBarVisible())
  {
    overlay_window_->ResetStates();
    ResetConnector();
  }
}

void PlacesOverlayVScrollBar::OnVisibilityChanged(nux::Area* /*area*/, bool visible)
{
  if (overlay_window_->IsVisible() && !visible)
  {
    overlay_window_->ResetStates();
    ResetConnector();
  }
}

void PlacesOverlayVScrollBar::OnSensitivityChanged(nux::Area* /*area*/, bool sensitive)
{
  if (!sensitive)
  {
    overlay_window_->ResetStates();
    ResetConnector();
  }
}

void PlacesOverlayVScrollBar::SetupAnimation(int start, int stop, int milliseconds)
{
  tweening_connection_.disconnect();
  delta_update_ = 0;

  animation_.SetDuration(milliseconds);
  animation_.SetEasingCurve(nux::animation::EasingCurve(nux::animation::EasingCurve::Type::Linear));

  animation_.SetStartValue(start);
  animation_.SetFinishValue(stop);
}

void PlacesOverlayVScrollBar::StartScrollAnimation(ScrollDir dir, int stop)
{
  if (animation_.CurrentState() == nux::animation::Animation::State::Stopped)
  {
    SetupAnimation(0, stop, SCROLL_ANIMATION);

    tweening_connection_ = animation_.updated.connect([this, dir] (int const& update) {
      OnScroll(dir, update - delta_update_);
      delta_update_ = update;

      CheckIfThumbIsInsideSlider();
      UpdateConnectorPosition();
      QueueDraw();
    });

    animation_.Start();
  }
}

void PlacesOverlayVScrollBar::OnScroll(ScrollDir dir, int mouse_dy)
{
  if (dir == ScrollDir::UP)
    OnScrollUp.emit(stepY, mouse_dy);
  else if (dir == ScrollDir::DOWN)
    OnScrollDown.emit(stepY, mouse_dy);
}

void PlacesOverlayVScrollBar::StartConnectorAnimation()
{
  if (animation_.CurrentState() == nux::animation::Animation::State::Stopped)
  {
    SetupAnimation(connector_height_, 0, std::min(connector_height_, MAX_CONNECTOR_ANIMATION));

    tweening_connection_ = animation_.updated.connect([this] (int const& update) {
      connector_height_ = update;
      UpdateConnectorTexture();
    });

    animation_.Start();
  }
}

bool PlacesOverlayVScrollBar::IsScrollBarVisible() const
{
  return (content_height_ > container_height_);
}

void PlacesOverlayVScrollBar::OnMouseEnter(int x, int y, unsigned int button_flags, unsigned int key_flags)
{
  overlay_window_->MouseEnter();
  UpdateConnectorPosition();
}

void PlacesOverlayVScrollBar::OnMouseLeave(int x, int y, unsigned int button_flags, unsigned int key_flags)
{
  overlay_window_->MouseLeave();
  UpdateConnectorPosition();
}

void PlacesOverlayVScrollBar::OnMouseNear(nux::Point const& mouse_pos)
{
  if (IsSensitive() && IsVisible() && IsScrollBarVisible())
  {
    animation_.Stop();

    overlay_window_->MouseNear();
    AdjustThumbOffsetFromMouse();
  }
}

void PlacesOverlayVScrollBar::OnMouseBeyond(nux::Point const& mouse_pos)
{
  if (IsVisible() && IsScrollBarVisible())
  {
    overlay_window_->MouseBeyond();
    UpdateConnectorPosition();
  }
}

void PlacesOverlayVScrollBar::AdjustThumbOffsetFromMouse()
{
  if (!overlay_window_->IsMouseBeingDragged())
  {
    nux::Point const& mouse = nux::GetWindowCompositor().GetMousePosition();

    if (mouse.y > 0)
    {
      int const quarter_of_thumb = overlay_window_->GetThumbHeight()/4;
      int const new_offset = mouse.y - _track->GetAbsoluteY() - overlay_window_->GetThumbHeight()/2;

      int const slider_offset = _slider->GetAbsoluteY() - _track->GetAbsoluteY() + _slider->GetBaseHeight()/3;
      bool const mouse_above_slider = slider_offset < new_offset;

      if (mouse_above_slider)
        overlay_window_->SetThumbOffsetY(new_offset - quarter_of_thumb);
      else
        overlay_window_->SetThumbOffsetY(new_offset + quarter_of_thumb);
    }

    CheckIfThumbIsInsideSlider();
  }
}

void PlacesOverlayVScrollBar::CheckIfThumbIsInsideSlider()
{
  nux::Geometry const& slider_geo = _slider->GetAbsoluteGeometry();
  nux::Geometry const& thumb_geo = overlay_window_->GetThumbGeometry();
  nux::Geometry const& intersection = (thumb_geo.Intersect(slider_geo));

  if (!intersection.IsNull())
  {
    ResetConnector();
    overlay_window_->ThumbInsideSlider();
  }
  else
  {
    UpdateConnectorPosition();
    overlay_window_->ThumbOutsideSlider();
  }
}

void PlacesOverlayVScrollBar::UpdateConnectorPosition()
{
  int const slider_y = _slider->GetBaseY() - GetBaseY();
  int const thumb_y = overlay_window_->GetThumbOffsetY();
  int const thumb_height = overlay_window_->GetThumbHeight();

  if (!overlay_window_->IsVisible())
  {
    ResetConnector();
  }
  else if (slider_y > thumb_y)
  {
    thumb_above_slider_ = true;
    connector_height_ = slider_y - (thumb_y + thumb_height);
  }
  else
  {
    thumb_above_slider_ = false;
    connector_height_ = thumb_y - (_slider->GetBaseY() + _slider->GetBaseHeight()) + _track->GetBaseY();
  }

  UpdateConnectorTexture();
}

void PlacesOverlayVScrollBar::ResetConnector()
{
  if (animation_.CurrentState() == nux::animation::Animation::State::Stopped)
  {
    if (connector_height_ > 0)
    {
      StartConnectorAnimation();
    }
  }
  else
  {
    connector_height_ = 0;
  }

  QueueDraw();
}

void PlacesOverlayVScrollBar::OnMouseClick(int /*x*/, int y, unsigned int button_flags, unsigned int /*key_flags*/)
{
  if (!overlay_window_->IsMouseBeingDragged())
  {
    int const button = nux::GetEventButton(button_flags);

    if (button == 1)
      LeftMouseClick(y);
    else if (button == 2)
      MiddleMouseClick(y);
  }

  overlay_window_->MouseUp();
}

void PlacesOverlayVScrollBar::LeftMouseClick(int y)
{
  if (IsMouseInTopHalfOfThumb(y))
  {
    int const top = _slider->GetBaseY() - _track->GetBaseY();
    StartScrollAnimation(ScrollDir::UP, std::min(_slider->GetBaseHeight(), top));
  }
  else
  {
    int const bottom = (_track->GetBaseY() + _track->GetBaseHeight()) -
                       (_slider->GetBaseHeight() + _slider->GetBaseY());
    StartScrollAnimation(ScrollDir::DOWN, std::min(_slider->GetBaseHeight(), bottom));
  }

  UpdateConnectorPosition();
}

void PlacesOverlayVScrollBar::MiddleMouseClick(int y)
{
  int const slider_offset = _slider->GetBaseY() - _track->GetBaseY();
  bool const move_up = slider_offset > overlay_window_->GetThumbOffsetY();

  int const slider_thumb_diff = abs(overlay_window_->GetThumbOffsetY() - slider_offset);

  if (move_up)
    StartScrollAnimation(ScrollDir::UP, slider_thumb_diff);
  else
    StartScrollAnimation(ScrollDir::DOWN, slider_thumb_diff);
}

void PlacesOverlayVScrollBar::OnMouseDown(int /*x*/, int y, unsigned int /*button_flags*/, unsigned int /*key_flags*/)
{
  if (overlay_window_->IsMouseInsideThumb(y))
  {
    if (IsMouseInTopHalfOfThumb(y))
      overlay_window_->PageUpAction();
    else
      overlay_window_->PageDownAction();

    mouse_down_offset_ = y - overlay_window_->GetThumbOffsetY();
    overlay_window_->MouseDown();
  }
}

bool PlacesOverlayVScrollBar::IsMouseInTopHalfOfThumb(int y)
{
  int const thumb_height = overlay_window_->GetThumbHeight();
  int const thumb_offset_y = overlay_window_->GetThumbOffsetY();

  return (y < (thumb_height/2 + thumb_offset_y));
}

void PlacesOverlayVScrollBar::OnMouseUp(int x, int y, unsigned int /*button_flags*/, unsigned int /*key_flags*/)
{
  nux::Geometry const& geo = overlay_window_->GetAbsoluteGeometry();

  if (!geo.IsPointInside(x + geo.x, y + geo.y))
  {
    overlay_window_->MouseUp();
    UpdateConnectorPosition();
  }
}

void PlacesOverlayVScrollBar::OnMouseMove(int /*x*/, int y, int /*dx*/, int /*dy*/, unsigned int /*button_flags*/, unsigned int /*key_flags*/)
{
  if (!overlay_window_->IsMouseInsideThumb(y))
    AdjustThumbOffsetFromMouse();
}

void PlacesOverlayVScrollBar::OnMouseDrag(int /*x*/, int y, int /*dx*/, int dy, unsigned int /*button_flags*/, unsigned int /*key_flags*/)
{
  animation_.Stop();
  MouseDraggingOverlay(y, dy);
}

void PlacesOverlayVScrollBar::MouseDraggingOverlay(int y, int dy)
{
  int const thumb_offset = overlay_window_->GetThumbOffsetY() + mouse_down_offset_;

  if (dy < 0 && !AtMinimum() && y <= thumb_offset)
  {
    OnScrollUp.emit(stepY, abs(dy));
  }
  else if (dy > 0 && !AtMaximum() && y >= thumb_offset)
  {
    OnScrollDown.emit(stepY, dy);
  }

  overlay_window_->SetThumbOffsetY(y - mouse_down_offset_);
  CheckIfThumbIsInsideSlider();
}

void PlacesOverlayVScrollBar::UpdateStepY()
{
  stepY = (float) (content_height_ - container_height_) / (float) (_track->GetBaseHeight() - _slider->GetBaseHeight());
}

void PlacesOverlayVScrollBar::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  PlacesVScrollBar::Draw(graphics_engine, force_draw);

  if (connector_height_ > 0 && connector_texture_.IsValid())
  {
    int const connector_width = GetBaseWidth();
    int offset_y = 0;
    if (thumb_above_slider_)
    {
      offset_y = _slider->GetBaseY() - connector_height_;
    }
    else
    {
      offset_y = _slider->GetBaseY() + _slider->GetBaseHeight();
    }

    nux::Geometry base(_track->GetBaseX(), offset_y - 4, connector_width, connector_height_ + 5);
    nux::TexCoordXForm texxform;

    graphics_engine.QRP_1Tex(base.x,
                             base.y,
                             base.width,
                             base.height,
                             connector_texture_->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
  }
}

void PlacesOverlayVScrollBar::UpdateConnectorTexture()
{
  if (connector_height_ < 0)
    return;

  int width = 3;
  int height = connector_height_;
  float const radius = 1.5f;
  float const aspect = 1.0f;

  cairo_t* cr = NULL;

  nux::color::RedGreenBlue const& connector_bg = nux::color::Gray;

  nux::CairoGraphics cairoGraphics(CAIRO_FORMAT_ARGB32, width, height);
  cr = cairoGraphics.GetContext();
  cairo_save(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_save(cr);

  cairo_set_source_rgba(cr, connector_bg.red, connector_bg.green, connector_bg.blue, 0.8);
  cairoGraphics.DrawRoundedRectangle(cr, aspect, 0.0f, 0.0f, radius, width, height);
  cairo_fill_preserve(cr);

  connector_texture_.Adopt(texture_from_cairo_graphics(cairoGraphics));
  cairo_destroy(cr);

  QueueDraw();
}

} // namespace dash
} // namespace unity

