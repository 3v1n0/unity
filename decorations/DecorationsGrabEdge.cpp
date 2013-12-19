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

GrabEdge::GrabEdge(CompWindow* win)
  : Edge(win, Edge::Type::CENTER)
  , last_click_time_(0)
{}

void GrabEdge::ButtonDownEvent(CompPoint const& p, unsigned button)
{
  if (button != 1)
    return;

  auto const& style = Style::Get();
  bool maximized = false;
  unsigned max_time_delta = std::max(0, style->DoubleClickMaxTimeDelta());

  if (screen->getCurrentTime() - last_click_time_ < max_time_delta)
  {
    int max_distance = style->DoubleClickMaxDistance();

    if (std::abs(p.x() - last_click_pos_.x()) < max_distance &&
        std::abs(p.y() - last_click_pos_.y()) < max_distance)
    {
      win_->maximize(MAXIMIZE_STATE);
      maximized = true;
    }
  }

  if (!maximized)
    Edge::ButtonDownEvent(p, button);

  last_click_pos_ = p;
  last_click_time_ = screen->getCurrentTime();
}

} // decoration namespace
} // unity namespace
