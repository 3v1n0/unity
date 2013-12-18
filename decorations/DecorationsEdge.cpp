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

#include <X11/cursorfont.h>
#include "DecorationsEdge.h"

namespace unity
{
namespace decoration
{

Edge::Edge(CompWindow* win, Type t)
  : win_(win)
  , type_(t)
  , cursor_(XCreateFontCursor(screen->dpy(), TypeToCursorShape(type_)))
{
  mouse_owner.changed.connect([this] (bool over) {
    if (over)
      XDefineCursor(screen->dpy(), win_->frame(), cursor_);
    else
      XUndefineCursor(screen->dpy(), win_->frame());
  });
}

Edge::~Edge()
{
  XFreeCursor(screen->dpy(), cursor_);
}

Edge::Type Edge::GetType() const
{
  return type_;
}

unsigned Edge::TypeToCursorShape(Edge::Type type)
{
  switch (type)
  {
    case Edge::Type::TOP:
      return XC_top_side;
    case Edge::Type::TOP_LEFT:
      return XC_top_left_corner;
    case Edge::Type::TOP_RIGHT:
      return XC_top_right_corner;
    case Edge::Type::LEFT:
      return XC_left_side;
    case Edge::Type::RIGHT:
      return XC_right_side;
    case Edge::Type::BOTTOM:
      return XC_bottom_side;
    case Edge::Type::BOTTOM_LEFT:
      return XC_bottom_left_corner;
    case Edge::Type::BOTTOM_RIGHT:
      return XC_bottom_right_corner;
    default:
      return XC_arrow;
  }
}

} // decoration namespace
} // unity namespace
