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

#ifndef UNITY_DECORATIONS_EDGE
#define UNITY_DECORATIONS_EDGE

#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class Edge : public SimpleItem
{
public:
  enum class Type
  {
    // The order of this enum is important to define the priority of each Edge
    // when parsing the input events (in case two areas overlap)
    CENTER = 0,
    TOP_LEFT,
    TOP_RIGHT,
    TOP,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    BOTTOM,
    LEFT,
    RIGHT,
    Size
  };

  Edge(CompWindow* win, Type t);

  Type GetType() const;
  void ButtonDownEvent(CompPoint const&, unsigned button);

protected:
  CompWindow* win_;
  Type type_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_EDGE
