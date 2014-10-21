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

#include "DecorationsEdgeBorders.h"
#include "DecorationsGrabEdge.h"

namespace unity
{
namespace decoration
{
namespace
{
const RawPixel MIN_CORNER_EDGE = 10_em;
}

EdgeBorders::EdgeBorders(CompWindow* win)
{
  scale.changed.connect(sigc::hide(sigc::mem_fun(this, &EdgeBorders::Relayout)));

  if (win->actions() & CompWindowActionResizeMask)
  {
    items_.resize(size_t(Edge::Type::Size));

    for (unsigned i = 0; i < unsigned(Edge::Type::Size); ++i)
    {
      auto type = Edge::Type(i);

      if (type == Edge::Type::GRAB)
        items_[i] = std::make_shared<GrabEdge>(win);
      else
        items_[i] = std::make_shared<Edge>(win, type);
    }
  }
  else /*if (win->actions() & CompWindowActionMoveMask)*/
  {
    items_.resize(size_t(Edge::Type::GRAB) + 1);
    items_[unsigned(Edge::Type::GRAB)] = std::make_shared<GrabEdge>(win);
  }

  Relayout();
}

void EdgeBorders::DoRelayout()
{
  auto const& grab_edge = items_[unsigned(Edge::Type::GRAB)];
  CompWindow* win = std::static_pointer_cast<GrabEdge>(grab_edge)->Window();
  auto const& b = win->border();
  auto const& ib = win->input();

  using namespace compiz::window::extents;
  int min_corner_edge = MIN_CORNER_EDGE.CP(scale);
  Extents edges(std::max(ib.left, min_corner_edge),
                std::max(ib.right, min_corner_edge),
                std::max(ib.top, min_corner_edge),
                std::max(ib.bottom, min_corner_edge));

  grab_edge->SetCoords(rect_.x() + ib.left, rect_.y() + ib.top - b.top);
  grab_edge->SetSize(rect_.width() - ib.left - ib.right, b.top);

  if (items_.size() != size_t(Edge::Type::Size))
    return;

  auto item = items_[unsigned(Edge::Type::TOP)];
  item->SetCoords(rect_.x() + edges.left, rect_.y());
  item->SetSize(rect_.width() - edges.left - edges.right, edges.top - b.top);

  item = items_[unsigned(Edge::Type::TOP_LEFT)];
  item->SetCoords(rect_.x(), rect_.y());
  item->SetSize(edges.left, edges.top);

  item = items_[unsigned(Edge::Type::TOP_RIGHT)];
  item->SetCoords(rect_.x2() - edges.right, rect_.y());
  item->SetSize(edges.right, edges.top);

  item = items_[unsigned(Edge::Type::LEFT)];
  item->SetCoords(rect_.x(), rect_.y() + edges.top);
  item->SetSize(edges.left, rect_.height() - edges.top - edges.bottom);

  item = items_[unsigned(Edge::Type::RIGHT)];
  item->SetCoords(rect_.x2() - edges.right, rect_.y() + edges.top);
  item->SetSize(edges.right, rect_.height() - edges.top - edges.bottom);

  item = items_[unsigned(Edge::Type::BOTTOM)];
  item->SetCoords(rect_.x() + edges.left, rect_.y2() - edges.bottom);
  item->SetSize(rect_.width() - edges.left - edges.right, edges.bottom);

  item = items_[unsigned(Edge::Type::BOTTOM_LEFT)];
  item->SetCoords(rect_.x(), rect_.y2() - edges.bottom);
  item->SetSize(edges.left, edges.bottom);

  item = items_[unsigned(Edge::Type::BOTTOM_RIGHT)];
  item->SetCoords(rect_.x2() - edges.right, rect_.y2() - edges.bottom);
  item->SetSize(edges.right, edges.bottom);
}

Item::Ptr const& EdgeBorders::GetEdge(Edge::Type type) const
{
  return items_[unsigned(type)];
}

} // decoration namespace
} // unity namespace
