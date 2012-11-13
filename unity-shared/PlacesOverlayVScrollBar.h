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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef PLACES_OVERLAY_VSCROLLBAR_H
#define PLACES_OVERLAY_VSCROLLBAR_H

#include <Nux/Nux.h>
#include <Nux/ProximityArea.h>
#include <NuxCore/Animation.h>
#include <memory>

#include "unity-shared/PlacesVScrollBar.h"
#include "unity-shared/VScrollBarOverlayWindow.h"

namespace unity
{
namespace dash
{

class PlacesOverlayVScrollBar: public PlacesVScrollBar
{
public:
  PlacesOverlayVScrollBar(NUX_FILE_LINE_PROTO);
  ~PlacesOverlayVScrollBar();

private:
  enum class ScrollDir : unsigned int
  {
    UP,
    DOWN,
  };

  void OnMouseNear(nux::Point mouse_pos);
  void OnMouseBeyond(nux::Point mouse_pos);
  void AdjustThumbOffsetFromMouse();

  void OnMouseClick(int x, int y, unsigned int button_flags, unsigned int key_flags);
  void LeftMouseClick(int y);
  void MiddleMouseClick(int y);

  void OnMouseDown(int x, int y, unsigned int button_flags, unsigned int key_flags);
  void OnMouseUp(int x, int y, unsigned int button_flags, unsigned int key_flags);

  void OnMouseMove(int x, int y, int dx, int dy, unsigned int button_flags, unsigned int key_flags);
  
  void OnMouseDrag(int x, int y, int dx, int dy, unsigned int button_flags, unsigned int key_flags);
  void MouseDraggingOverlay(int y, int dy);

  bool IsMouseInTopHalfOfThumb(int y);
  bool IsMouseInTrackRange(int y);

  void CheckIfThumbIsInsideSlider();
  
  void UpdateStepY();

  void SetupAnimation(ScrollDir dir, int stop);
  void StartAnimation(ScrollDir dir);
  void OnScroll(ScrollDir dir, int mouse_dy);

private:
  nux::ObjectPtr<VScrollBarOverlayWindow> _overlay_window;
  std::shared_ptr<nux::ProximityArea> _prox_area;

  nux::animation::AnimateValue<int> _animation;
  sigc::connection _tweening_connection;

  int _mouse_down_offset;
};

} // namespace dash
} // namespace unity

#endif // PLACES_OVERLAY_VSCROLLBAR_H
