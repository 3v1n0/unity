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
    TOP = 0,
    TOP_LEFT,
    TOP_RIGHT,
    LEFT,
    RIGHT,
    BOTTOM,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    Size
  };

  Edge(CompWindow* win, Type t);
  ~Edge();

  Type GetType() const;

  static unsigned TypeToCursorShape(Type);

private:
  CompWindow* win_;
  Type type_;
  Cursor cursor_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_EDGE
