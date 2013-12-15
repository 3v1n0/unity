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

#ifndef UNITY_DECORATION_STYLE
#define UNITY_DECORATION_STYLE

#include <memory>
#include <cairo/cairo.h>

namespace unity
{
namespace decoration
{

enum class Side : unsigned
{
  TOP = 0,
  LEFT,
  RIGHT,
  BOTTOM,
};

enum class Alignment
{
  LEFT,
  CENTER,
  RIGHT,
  FLOATING
};

enum class WidgetState : unsigned
{
  NORMAL,
  PRELIGHT,
  PRESSED,
  DISABLED,
  BACKDROP,
  BACKDROP_PRELIGHT,
  BACKDROP_PRESSED,
  Size
};

enum class WindowButtonType : unsigned
{
  CLOSE,
  MINIMIZE,
  UNMAXIMIZE,
  MAXIMIZE,
  Size
};

struct Border
{
  Border(int top, int left, int right, int bottom)
    : top(top)
    , left(left)
    , right(right)
    , bottom(bottom)
  {}

  Border()
    : Border(0, 0, 0, 0)
  {}

  int top;
  int left;
  int right;
  int bottom;
};

class Style
{
public:
  typedef std::shared_ptr<Style> Ptr;

  static Style::Ptr const& Get();
  virtual ~Style();

  Alignment TitleAlignment() const;
  float TitleAlignmentValue() const;
  decoration::Border const& Border() const;
  int BorderWidth(Side) const;
  decoration::Border Padding(Side, WidgetState ws = WidgetState::NORMAL) const;
  std::string WindowButtonFile(WindowButtonType, WidgetState) const;

  void DrawSide(Side, WidgetState, cairo_t*, int width, int height);

private:
  Style();
  Style(Style const&) = delete;
  Style& operator=(Style const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // decoration namespace
} // unity namespace

#endif
