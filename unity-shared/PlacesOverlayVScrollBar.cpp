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
#include <Nux/InputAreaProximity.h>

namespace
{
  int const PROXIMITY = 7;
  int const ANIMATION_TIME = 400;
}

namespace unity
{
namespace dash
{

PlacesOverlayVScrollBar::PlacesOverlayVScrollBar(NUX_FILE_LINE_DECL)
  : PlacesVScrollBar(NUX_FILE_LINE_PARAM)
  , _overlay_window(new VScrollBarOverlayWindow(_track->GetAbsoluteGeometry()))
  , _mouse_down_offset(0)
{
  _area_prox.reset(new nux::InputAreaProximity(_overlay_window.GetPointer(), PROXIMITY));
  _area_prox->mouse_near.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseNear));
  _area_prox->mouse_beyond.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseBeyond));

  _overlay_window->mouse_down.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseDown));
  _overlay_window->mouse_up.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseUp));
  _overlay_window->mouse_click.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseClick));
  _overlay_window->mouse_move.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseMove));
  _overlay_window->mouse_drag.connect(sigc::mem_fun(this, &PlacesOverlayVScrollBar::OnMouseDrag));

  _track->geometry_changed.connect([&] (nux::Area* area, nux::Geometry& geo) {
    UpdateStepY();
    _overlay_window->UpdateGeometry(_track->GetAbsoluteGeometry());

    if (_overlay_window->IsVisible() && content_height_ <= container_height_)
      _overlay_window->ResetStates();
  });

  OnVisibleChanged.connect([&] (nux::Area* area, bool visible) {
    _overlay_window->SetVisible(visible);
  });
}

PlacesOverlayVScrollBar::~PlacesOverlayVScrollBar()
{
}

void PlacesOverlayVScrollBar::SetupAnimation(ScrollDir dir, int stop)
{
  if (_animation.CurrentState() == nux::animation::Animation::State::Stopped)
  {
    _tweening_connection.disconnect();

    _animation.SetDuration(ANIMATION_TIME);
    _animation.SetEasingCurve(nux::animation::EasingCurve(nux::animation::EasingCurve::Type::Linear));

    _animation.SetStartValue(0);
    _animation.SetFinishValue(stop);

    StartAnimation(dir);
  }
}

void PlacesOverlayVScrollBar::StartAnimation(ScrollDir dir)
{
  _tweening_connection = _animation.updated.connect([this, dir] (int const& update) {
    static int delta_update = 0;

    OnScroll(dir, update - delta_update);
    delta_update = update;

    CheckIfThumbIsInsideSlider();
    QueueDraw();
    if (update == _animation.GetFinishValue())
      delta_update = 0;
  });

  _animation.Start();
}

void PlacesOverlayVScrollBar::OnScroll(ScrollDir dir, int mouse_dy)
{
  if (dir == ScrollDir::UP)
    OnScrollUp.emit(stepY, mouse_dy);
  else if (dir == ScrollDir::DOWN)
    OnScrollDown.emit(stepY, mouse_dy);
}

void PlacesOverlayVScrollBar::OnMouseNear(nux::Point const& mouse_pos)
{
  if (IsVisible() && content_height_ > container_height_)
  {
    _overlay_window->MouseNear();
    AdjustThumbOffsetFromMouse();
  }
}

void PlacesOverlayVScrollBar::OnMouseBeyond(nux::Point const& mouse_pos)
{
  if (IsVisible() && content_height_ > container_height_)
    _overlay_window->MouseBeyond();
}

void PlacesOverlayVScrollBar::AdjustThumbOffsetFromMouse()
{
  const nux::Point& mouse = nux::GetWindowCompositor().GetMousePosition();
  const int new_offset = mouse.y - _track->GetAbsoluteY() - _overlay_window->GetThumbHeight()/2;

  const int slider_offset = _slider->GetBaseY() - _track->GetBaseY();
  const bool thumb_above_slider = slider_offset < new_offset;

  if (thumb_above_slider)
    _overlay_window->SetThumbOffsetY(new_offset - _overlay_window->GetThumbHeight()/4);
  else
    _overlay_window->SetThumbOffsetY(new_offset + _overlay_window->GetThumbHeight()/4);

  CheckIfThumbIsInsideSlider();
}

void PlacesOverlayVScrollBar::CheckIfThumbIsInsideSlider()
{
  nux::Geometry const& slider_geo = _slider->GetAbsoluteGeometry();
  nux::Geometry const& thumb_geo = _overlay_window->GetThumbGeometry();
  nux::Geometry const& intersection = (thumb_geo.Intersect(slider_geo));

  if (!intersection.IsNull())
    _overlay_window->ThumbInsideSlider();
  else
    _overlay_window->ThumbOutsideSlider();
}

void PlacesOverlayVScrollBar::OnMouseClick(int x, int y, unsigned int button_flags, unsigned int key_flags)
{
  if (!_overlay_window->IsMouseBeingDragged())
  {
    int const button = nux::GetEventButton(button_flags);

    if (button == 1)
      LeftMouseClick(y);
    else if (button == 2)
      MiddleMouseClick(y);
  }

  _overlay_window->MouseUp();
}

void PlacesOverlayVScrollBar::LeftMouseClick(int y)
{
  if (IsMouseInTopHalfOfThumb(y))
  {
    int const top = _slider->GetBaseY() - _track->GetBaseY();
    SetupAnimation(ScrollDir::UP, std::min(_slider->GetBaseHeight(), top));
  }
  else
  {
    int const bottom = (_track->GetBaseY() + _track->GetBaseHeight()) - 
                       (_slider->GetBaseHeight() + _slider->GetBaseY());
    SetupAnimation(ScrollDir::DOWN, std::min(_slider->GetBaseHeight(), bottom));
  }
}

void PlacesOverlayVScrollBar::MiddleMouseClick(int y)
{
  int const slider_offset = _slider->GetBaseY() - _track->GetBaseY();
  bool const move_up = slider_offset > _overlay_window->GetThumbOffsetY();

  int const slider_thumb_diff = abs(_overlay_window->GetThumbOffsetY() - slider_offset);

  if (move_up)
    SetupAnimation(ScrollDir::UP, slider_thumb_diff);
  else
    SetupAnimation(ScrollDir::DOWN, slider_thumb_diff);
}

void PlacesOverlayVScrollBar::OnMouseDown(int x, int y, unsigned int button_flags, unsigned int key_flags)
{
  if (_overlay_window->IsMouseInsideThumb(y))
  {
    if (IsMouseInTopHalfOfThumb(y))
      _overlay_window->PageUpAction();
    else
      _overlay_window->PageDownAction();

    _mouse_down_offset = y - _overlay_window->GetThumbOffsetY();
    _overlay_window->MouseDown();
  }
}

bool PlacesOverlayVScrollBar::IsMouseInTopHalfOfThumb(int y)
{
  int const thumb_height = _overlay_window->GetThumbHeight();
  int const thumb_offset_y = _overlay_window->GetThumbOffsetY();

  return (y < (thumb_height/2 + thumb_offset_y));
}

void PlacesOverlayVScrollBar::OnMouseUp(int x, int y, unsigned int button_flags, unsigned int key_flags)
{
  nux::Geometry const& geo = _overlay_window->GetAbsoluteGeometry();

  if (!geo.IsPointInside(x + geo.x, y + geo.y))
    _overlay_window->MouseUp();
}

void PlacesOverlayVScrollBar::OnMouseMove(int x, int y, int dx, int dy, unsigned int button_flags, unsigned int key_flags)
{
  if (!_overlay_window->IsMouseInsideThumb(y))
    AdjustThumbOffsetFromMouse();
}

void PlacesOverlayVScrollBar::OnMouseDrag(int x, int y, int dx, int dy, unsigned int button_flags, unsigned int key_flags)
{
  MouseDraggingOverlay(y, dy);
}

void PlacesOverlayVScrollBar::MouseDraggingOverlay(int y, int dys)
{
  int const dy = y - _overlay_window->GetThumbOffsetY() - _mouse_down_offset;
  int const at_min = _overlay_window->GetThumbOffsetY() <= 0;
  int const at_max = _overlay_window->GetThumbOffsetY() + _overlay_window->GetThumbHeight() >= _track->GetBaseHeight();

  if (dy < 0 && !at_min)
  {
    OnScrollUp.emit(stepY, abs(dy));
  }
  else if (dy > 0 && !at_max)
  {
    OnScrollDown.emit(stepY, dy);
  }

  _overlay_window->SetThumbOffsetY(y - _mouse_down_offset);
  CheckIfThumbIsInsideSlider();
}

void PlacesOverlayVScrollBar::UpdateStepY()
{
  stepY = (float) (content_height_ - container_height_) / (float) (_track->GetBaseHeight() - _slider->GetBaseHeight());
}

} // namespace dash
} // namespace unity

