// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2015 Canonical Ltd
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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <Nux/Nux.h>

#include "AnimationUtils.h"
#include "DashStyle.h"
#include "PlacesOverlayVScrollBar.h"
#include "UnitySettings.h"

namespace unity
{
namespace dash
{
namespace
{
const RawPixel PROXIMITY = 7_em;
const int PAGE_SCROLL_ANIMATION = 200;
const int CLICK_SCROLL_ANIMATION = 80;
}

struct PlacesOverlayVScrollBar::ProximityArea : public nux::InputAreaProximity, public sigc::trackable
{
  ProximityArea(nux::InputArea* area, unsigned prox)
    : nux::InputAreaProximity(area, prox)
    , proximity([this] { return proximity_; }, [this] (unsigned px) { proximity_ = px; return false; })
  {}

  nux::RWProperty<unsigned> proximity;

  bool IsMouseNear() const { return is_mouse_near_; }
};

PlacesOverlayVScrollBar::PlacesOverlayVScrollBar(NUX_FILE_LINE_DECL)
  : PlacesVScrollBar(NUX_FILE_LINE_PARAM)
  , area_prox_(std::make_shared<ProximityArea>(this, PROXIMITY.CP(scale)))
  , delta_update_(0)
{
  scale.changed.connect([this] (double scale) {
    area_prox_->proximity = PROXIMITY.CP(scale);
    UpdateScrollbarSize();
  });

  auto update_sb_cb = sigc::mem_fun(this, &PlacesOverlayVScrollBar::UpdateScrollbarSize);
  Style::Instance().changed.connect(update_sb_cb);

  auto update_sb_proximity_cb = sigc::hide(update_sb_cb);
  area_prox_->mouse_near.connect(update_sb_proximity_cb);
  area_prox_->mouse_beyond.connect(update_sb_proximity_cb);

  auto update_sb_mouse_cb = sigc::hide(sigc::hide(sigc::hide(update_sb_proximity_cb)));
  _track->mouse_enter.connect(update_sb_mouse_cb);
  _track->mouse_leave.connect(update_sb_mouse_cb);
  _slider->mouse_enter.connect(update_sb_mouse_cb);
  _slider->mouse_leave.connect(update_sb_mouse_cb);
  _slider->mouse_up.connect(update_sb_mouse_cb);
  _track->mouse_up.connect(update_sb_mouse_cb);

  // Disable default track-click handlers
  _track->mouse_down.clear();
  _track->mouse_up.clear();
  _track->mouse_down.connect([this] (int x, int y, unsigned long, unsigned long) {
    int slider_vcenter = _slider->GetBaseY() - _track->GetBaseY() + _slider->GetBaseHeight() / 2;
    int scroll_offset = slider_vcenter - y;

    if (scroll_offset > 0)
      StartScrollAnimation(ScrollDir::UP, scroll_offset, CLICK_SCROLL_ANIMATION);
    else
      StartScrollAnimation(ScrollDir::DOWN, -scroll_offset, CLICK_SCROLL_ANIMATION);
  });

  UpdateScrollbarSize();
}

void PlacesOverlayVScrollBar::UpdateScrollbarSize()
{
  bool is_hovering = false;
  auto& style = Style::Instance();

  int active_width = style.GetScrollbarSize().CP(scale);
  SetMinimumWidth(active_width);
  SetMaximumWidth(active_width);

  int buttons_height = style.GetScrollbarButtonsSize().CP(scale);
  _scroll_up_button->SetMaximumHeight(buttons_height);
  _scroll_up_button->SetMinimumHeight(buttons_height);
  _scroll_down_button->SetMaximumHeight(buttons_height);
  _scroll_down_button->SetMinimumHeight(buttons_height);

  int slider_width = style.GetOverlayScrollbarSize().CP(scale);

  if (_track->IsMouseInside() || _track->IsMouseOwner() ||
      _slider->IsMouseInside() || _slider->IsMouseOwner() ||
       area_prox_->IsMouseNear())
  {
    is_hovering = true;
    slider_width = active_width;
  }

  hovering = is_hovering;

  _slider->SetMinimumWidth(slider_width);
  _slider->SetMaximumWidth(slider_width);
  _scroll_up_button->SetBaseWidth(slider_width);

  QueueDraw();
}

void PlacesOverlayVScrollBar::PerformPageNavigation(ScrollDir dir)
{
  StartScrollAnimation(dir, _slider->GetBaseHeight(), Settings::Instance().low_gfx() ? 0 : PAGE_SCROLL_ANIMATION);
}

void PlacesOverlayVScrollBar::StartScrollAnimation(ScrollDir dir, int stop, unsigned duration)
{
  if (animation_.CurrentState() != nux::animation::Animation::State::Stopped)
    return;

  delta_update_ = 0;
  stepY = (float) (content_height_ - container_height_) / (float) (_track->GetBaseHeight() - _slider->GetBaseHeight());

  tweening_connection_ = animation_.updated.connect([this, dir] (int update) {
      int mouse_dy = update - delta_update_;

      if (dir == ScrollDir::UP)
        OnScrollUp.emit(stepY, mouse_dy);
      else if (dir == ScrollDir::DOWN)
        OnScrollDown.emit(stepY, mouse_dy);

      delta_update_ = update;
      QueueDraw();
    });

  animation_.SetDuration(duration);
  animation::Start(animation_, 0, stop);
}

} // namespace dash
} // namespace unity
