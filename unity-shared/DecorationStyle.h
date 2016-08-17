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

#include <NuxCore/Property.h>
#include <NuxCore/Rect.h>
#include <cairo/cairo.h>

#include <memory>
#include <vector>

struct _GtkStyleContext;

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

enum class WMEvent
{
  DOUBLE_CLICK = 1,
  MIDDLE_CLICK = 2,
  RIGHT_CLICK = 3
};

enum class WMAction
{
  TOGGLE_SHADE,
  TOGGLE_MAXIMIZE,
  TOGGLE_MAXIMIZE_HORIZONTALLY,
  TOGGLE_MAXIMIZE_VERTICALLY,
  MINIMIZE,
  SHADE,
  MENU,
  LOWER,
  NONE
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
  ~Style();

  nux::ROProperty<std::string> theme;
  nux::ROProperty<std::string> font;
  nux::Property<std::string> title_font;
  nux::Property<unsigned> grab_wait;
  nux::Property<double> font_scale;

  decoration::Border const& Border() const;
  decoration::Border const& InputBorder() const;
  decoration::Border const& CornerRadius() const;
  decoration::Border Padding(Side, WidgetState ws = WidgetState::NORMAL) const;

  nux::Point ShadowOffset() const;
  nux::Color ActiveShadowColor() const;
  unsigned ActiveShadowRadius() const;
  nux::Color InactiveShadowColor() const;
  unsigned InactiveShadowRadius() const;

  unsigned GlowSize() const;
  nux::Color const& GlowColor() const;

  Alignment TitleAlignment() const;
  float TitleAlignmentValue() const;
  int TitleIndent() const;
  nux::Size TitleNaturalSize(std::string const&);
  nux::Size MenuItemNaturalSize(std::string const&);

  std::string ThemedFilePath(std::string const& basename, std::vector<std::string> const& extra_folders = {}) const;

  std::string WindowButtonFile(WindowButtonType, WidgetState) const;
  void DrawWindowButton(WindowButtonType, WidgetState, cairo_t*, double width, double height);

  WMAction WindowManagerAction(WMEvent) const;
  int DoubleClickMaxDistance() const;
  int DoubleClickMaxTimeDelta() const;

  void DrawSide(Side, WidgetState, cairo_t*, double width, double height);
  void DrawTitle(std::string const&, WidgetState, cairo_t*, double width, double height, nux::Rect const& bg_geo = nux::Rect(), _GtkStyleContext* ctx = nullptr);
  void DrawMenuItem(WidgetState, cairo_t*, double width, double height);
  void DrawMenuItemEntry(std::string const&, WidgetState, cairo_t*, double width, double height, nux::Rect const& bg_geo = nux::Rect());
  void DrawMenuItemIcon(std::string const&, WidgetState, cairo_t*, int size);

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
