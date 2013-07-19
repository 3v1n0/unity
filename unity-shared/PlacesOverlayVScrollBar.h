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

#ifndef PLACES_OVERLAY_VSCROLLBAR_H
#define PLACES_OVERLAY_VSCROLLBAR_H

#include <Nux/Nux.h>
#include <Nux/InputAreaProximity.h>
#include <NuxCore/Animation.h>
#include <UnityCore/ConnectionManager.h>
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

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw);

private:
  enum class ScrollDir : unsigned int
  {
    UP,
    DOWN,
  };

  void OnTrackGeometryChanged(nux::Area* area, nux::Geometry& geo);
  void OnVisibilityChanged(nux::Area* area, bool visible);
  void OnSensitivityChanged(nux::Area* area, bool sensitive);

  void OnMouseEnter(int x, int y, unsigned int button_flags, unsigned int key_flags);
  void OnMouseLeave(int x, int y, unsigned int button_flags, unsigned int key_flags);

  void OnMouseNear(nux::Point const& mouse_pos);
  void OnMouseBeyond(nux::Point const& mouse_pos);
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
  void CheckIfThumbIsInsideSlider();

  bool IsScrollBarVisible() const;

  void UpdateConnectorPosition();
  void ResetConnector();
  
  void UpdateStepY();

  void SetupAnimation(int start, int stop, int milliseconds);

  void StartScrollAnimation(ScrollDir dir, int stop);
  void OnScroll(ScrollDir dir, int mouse_dy);

  void StartConnectorAnimation();

  void UpdateConnectorTexture();

  nux::ObjectPtr<VScrollBarOverlayWindow> overlay_window_;
  nux::InputAreaProximity area_prox_;

  nux::animation::AnimateValue<int> animation_;
  connection::Wrapper tweening_connection_;

  nux::ObjectPtr<nux::BaseTexture> connector_texture_;
  
  bool thumb_above_slider_;
  int connector_height_;
  int mouse_down_offset_;
  int delta_update_;

  friend class MockScrollBar;
};

} // namespace dash
} // namespace unity

#endif // PLACES_OVERLAY_VSCROLLBAR_H
