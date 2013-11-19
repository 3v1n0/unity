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


#ifndef VSCROLLBAR_OVERLAY_WINDOW_H
#define VSCROLLBAR_OVERLAY_WINDOW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Animation.h>

namespace unity
{

class VScrollBarOverlayWindow : public nux::BaseWindow
{
public:
  VScrollBarOverlayWindow(nux::Geometry const& geo);
  virtual ~VScrollBarOverlayWindow() {}

  void UpdateGeometry(nux::Geometry const& geo);
  void SetThumbOffsetY(int y);

  void MouseDown();
  void MouseUp();

  void MouseNear();
  void MouseBeyond();

  void MouseEnter();
  void MouseLeave();

  void ThumbInsideSlider();
  void ThumbOutsideSlider();

  void PageUpAction();
  void PageDownAction();
  
  bool IsMouseInsideThumb(int y) const;
  bool IsMouseBeingDragged() const;

  int GetThumbHeight() const;
  int GetThumbOffsetY() const;

  nux::Geometry GetThumbGeometry() const;

  void ResetStates();

protected:
  virtual void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw);

private:
  enum class ThumbAction : unsigned int
  {
    NONE,
    DRAGGING,
    PAGE_UP,
    PAGE_DOWN
  };

  enum ThumbState
  {
    NONE          = 1 << 0,
    MOUSE_DOWN    = 1 << 1,
    MOUSE_NEAR    = 1 << 2,
    MOUSE_INSIDE  = 1 << 3,
    INSIDE_SLIDER = 1 << 4
  };

  void MouseDragging();
  void UpdateMouseOffsetX();
  int GetValidOffsetYValue(int y) const;

  void ShouldShow();
  void ShouldHide();

  void UpdateTexture();

  nux::Geometry content_size_;
  nux::ObjectPtr<nux::BaseTexture> thumb_texture_;
  
  int content_offset_x_;
  int mouse_offset_y_;

  void AddState(ThumbState const& state);
  void RemoveState(ThumbState const& state);
  bool HasState(ThumbState const& state) const;
  
  unsigned int current_state_;
  ThumbAction current_action_;
  nux::animation::AnimateValue<double> show_animator_;
};

} // namespace unity

#endif // VSCROLLBAR_OVERLAY_WINDOW_H
