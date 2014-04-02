// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <core/atoms.h>
#include "DecorationsGrabEdge.h"
#include "DecorationStyle.h"

namespace unity
{
namespace decoration
{

GrabEdge::GrabEdge(CompWindow* win, bool always_wait_grab_timeout)
  : Edge(win, Edge::Type::GRAB)
  , last_click_time_(0)
  , button_down_(-1)
  , always_wait_grab_timeout_(always_wait_grab_timeout)
{}

void GrabEdge::ButtonDownEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  if (button != 1)
  {
    if (button == 2 || button == 3)
      PerformWMAction(p, button, timestamp);

    return;
  }

  if (!IsMaximizable() && !always_wait_grab_timeout_)
  {
    Edge::ButtonDownEvent(p, button, timestamp);
    return;
  }

  auto const& style = Style::Get();
  unsigned max_time_delta = std::max(0, style->DoubleClickMaxTimeDelta());
  bool double_clicked = false;

  if (timestamp - last_click_time_ < max_time_delta)
  {
    int max_distance = style->DoubleClickMaxDistance();

    if (std::abs(p.x() - last_click_pos_.x()) < max_distance &&
        std::abs(p.y() - last_click_pos_.y()) < max_distance)
    {
      PerformWMAction(p, button, timestamp);
      double_clicked = true;
      button_down_timer_.reset();
    }
  }

  if (!double_clicked)
  {
    button_down_timer_.reset(new glib::Timeout(style->grab_wait()));
    button_down_timer_->Run([this] {
      Edge::ButtonDownEvent(CompPoint(pointerX, pointerY), button_down_, last_click_time_);
      button_down_timer_.reset();
      return false;
    });
  }

  button_down_ = button;
  last_click_pos_ = p;
  last_click_time_ = timestamp;
}

void GrabEdge::MotionEvent(CompPoint const& p, Time timestamp)
{
  if (button_down_timer_)
  {
    button_down_timer_.reset();
    Edge::ButtonDownEvent(p, button_down_, timestamp);
  }
}

void GrabEdge::ButtonUpEvent(CompPoint const&, unsigned button, Time)
{
  button_down_timer_.reset();
  button_down_ = -1;
}

void GrabEdge::PerformWMAction(CompPoint const& p, unsigned button, Time timestamp)
{
  WMAction action = Style::Get()->WindowManagerAction(WMEvent(button));

  switch (action)
  {
    case WMAction::TOGGLE_SHADE:
      if (win_->state() & CompWindowStateShadedMask)
        win_->changeState(win_->state() & ~CompWindowStateShadedMask);
      else
        win_->changeState(win_->state() | CompWindowStateShadedMask);

      win_->updateAttributes(CompStackingUpdateModeNone);
      break;
    case WMAction::TOGGLE_MAXIMIZE:
      if ((win_->state() & MAXIMIZE_STATE) == MAXIMIZE_STATE)
        win_->maximize(0);
      else
        win_->maximize(MAXIMIZE_STATE);
      break;
    case WMAction::TOGGLE_MAXIMIZE_HORIZONTALLY:
      if (win_->state() & CompWindowStateMaximizedHorzMask)
        win_->maximize(0);
      else
        win_->maximize(CompWindowStateMaximizedHorzMask);
      break;
    case WMAction::TOGGLE_MAXIMIZE_VERTICALLY:
    if (win_->state() & CompWindowStateMaximizedVertMask)
        win_->maximize(0);
      else
        win_->maximize(CompWindowStateMaximizedVertMask);
      break;
    case WMAction::MINIMIZE:
      win_->minimize();
      break;
    case WMAction::SHADE:
      win_->changeState(win_->state() | CompWindowStateShadedMask);
      win_->updateAttributes(CompStackingUpdateModeNone);
      break;
    case WMAction::MENU:
      screen->toolkitAction(Atoms::toolkitActionWindowMenu, timestamp, win_->id(), button, p.x(), p.y());
      break;
    case WMAction::LOWER:
      win_->lower();
      break;
    default:
      break;
  }
}

bool GrabEdge::IsGrabbed() const
{
  return !button_down_timer_;
}

int GrabEdge::ButtonDown() const
{
  return button_down_;
}

CompPoint const& GrabEdge::ClickedPoint() const
{
  return last_click_pos_;
}

bool GrabEdge::IsMaximizable() const
{
  return (win_->actions() & (CompWindowActionMaximizeHorzMask|CompWindowActionMaximizeVertMask));
}

void GrabEdge::AddProperties(debug::IntrospectionData& data)
{
  Edge::AddProperties(data);
  data.add("button_down", button_down_)
  .add("maximizable", IsMaximizable())
  .add("always_wait_grab_timeout", always_wait_grab_timeout_);
}

} // decoration namespace
} // unity namespace
