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
#include "DecorationsEdge.h"
#include "DecorationsDataPool.h"

namespace unity
{
namespace decoration
{
namespace
{

unsigned TypeToDirection(Edge::Type type)
{
  switch (type)
  {
    case Edge::Type::TOP:
      return WmMoveResizeSizeTop;
    case Edge::Type::TOP_LEFT:
      return WmMoveResizeSizeTopLeft;
    case Edge::Type::TOP_RIGHT:
      return WmMoveResizeSizeTopRight;
    case Edge::Type::LEFT:
      return WmMoveResizeSizeLeft;
    case Edge::Type::RIGHT:
      return WmMoveResizeSizeRight;
    case Edge::Type::BOTTOM:
      return WmMoveResizeSizeBottom;
    case Edge::Type::BOTTOM_LEFT:
      return WmMoveResizeSizeBottomLeft;
    case Edge::Type::BOTTOM_RIGHT:
      return WmMoveResizeSizeBottomRight;
    case Edge::Type::GRAB:
      return WmMoveResizeMove;
    default:
      return WmMoveResizeCancel;
  }
}

}

Edge::Edge(CompWindow* win, Type t)
  : win_(win)
  , type_(t)
{
  unsigned mask = (t == Type::GRAB) ? CompWindowActionMoveMask : CompWindowActionResizeMask;
  sensitive = (win_->actions() & mask);
  mouse_owner.changed.connect([this] (bool over) {
    if (over)
      XDefineCursor(screen->dpy(), win_->frame(), DataPool::Get()->EdgeCursor(type_));
    else
      XUndefineCursor(screen->dpy(), win_->frame());
  });
}

Edge::Type Edge::GetType() const
{
  return type_;
}

CompWindow* Edge::Window() const
{
  return win_;
}

void Edge::ButtonDownEvent(CompPoint const& p, unsigned button, Time timestamp)
{
  XEvent ev;
  auto* dpy = screen->dpy();

  ev.xclient.type = ClientMessage;
  ev.xclient.display = screen->dpy();

  ev.xclient.serial = 0;
  ev.xclient.send_event = True;

  ev.xclient.window = win_->id();
  ev.xclient.message_type = Atoms::wmMoveResize;
  ev.xclient.format = 32;

  ev.xclient.data.l[0] = p.x();
  ev.xclient.data.l[1] = p.y();
  ev.xclient.data.l[2] = TypeToDirection(type_);
  ev.xclient.data.l[3] = button;
  ev.xclient.data.l[4] = 1;

  XUngrabPointer(dpy, timestamp);
  XUngrabKeyboard(dpy, timestamp);

  auto mask = SubstructureRedirectMask | SubstructureNotifyMask;
  XSendEvent(dpy, screen->root(), False, mask, &ev);

  XSync(dpy, False);
}

std::string Edge::GetName() const
{
  switch (type_)
  {
    case Type::TOP:
      return "TopEdge";
    case Type::TOP_LEFT:
      return "TopLeftEdge";
    case Type::TOP_RIGHT:
      return "TopRightEdge";
    case Type::LEFT:
      return "LeftEdge";
    case Type::RIGHT:
      return "RightEdge";
    case Type::BOTTOM:
      return "BottomEdge";
    case Type::BOTTOM_LEFT:
      return "BottomLeftEdge";
    case Type::BOTTOM_RIGHT:
      return "BottomRightEdge";
    case Type::GRAB:
      return "GrabEdge";
    default:
      return "Edge";
  }
}

} // decoration namespace
} // unity namespace
