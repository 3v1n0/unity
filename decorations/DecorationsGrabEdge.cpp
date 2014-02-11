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

#include "DecorationsGrabEdge.h"
#include "DecorationStyle.h"

namespace unity
{
namespace decoration
{
namespace
{
const int MOUSE_DOWN_TIMEOUT = 150;
}

GrabEdge::GrabEdge(CompWindow* win, bool always_wait_grab_timeout)
  : Edge(win, Edge::Type::GRAB)
  , last_click_time_(0)
  , button_down_(-1)
  , always_wait_grab_timeout_(always_wait_grab_timeout)
{}

void GrabEdge::ButtonDownEvent(CompPoint const& p, unsigned button)
{
  if (button != 1)
    return;

  if (!IsMaximizable() && !always_wait_grab_timeout_)
  {
    Edge::ButtonDownEvent(p, button);
    return;
  }

  auto const& style = Style::Get();
  unsigned max_time_delta = std::max(0, style->DoubleClickMaxTimeDelta());
  bool maximized = false;

  if (screen->getCurrentTime() - last_click_time_ < max_time_delta)
  {
    int max_distance = style->DoubleClickMaxDistance();

    if (std::abs(p.x() - last_click_pos_.x()) < max_distance &&
        std::abs(p.y() - last_click_pos_.y()) < max_distance)
    {
      win_->maximize(MAXIMIZE_STATE);
      maximized = true;
      button_down_timer_.reset();
    }
  }

  if (!maximized)
  {
    button_down_timer_.reset(new glib::Timeout(MOUSE_DOWN_TIMEOUT));
    button_down_timer_->Run([this] {
      Edge::ButtonDownEvent(CompPoint(pointerX, pointerY), button_down_);
      button_down_timer_.reset();
      return false;
    });
  }

  button_down_ = button;
  last_click_pos_ = p;
  last_click_time_ = screen->getCurrentTime();
}

void GrabEdge::MotionEvent(CompPoint const& p)
{
  if (button_down_timer_)
  {
    button_down_timer_.reset();
    Edge::ButtonDownEvent(p, button_down_);
  }
}

void GrabEdge::ButtonUpEvent(CompPoint const&, unsigned button)
{
  button_down_timer_.reset();
  button_down_ = -1;
}

bool GrabEdge::IsGrabbed() const
{
  return !button_down_timer_;
}

int GrabEdge::ButtonDown() const
{
  return button_down_;
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
