// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 *              Mirco Müller <mirco.mueller@canonical.com
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>

#include "CairoBaseWindow.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/Introspectable.h"

namespace unity
{
class Tooltip : public CairoBaseWindow, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(Tooltip, CairoBaseWindow);
public:
  Tooltip(int monitor = 0);

  nux::RWProperty<std::string> text;

  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);
  void DrawContent(nux::GraphicsEngine& gfxContext, bool forceDraw);

  void SetTooltipPosition(int x, int y);
  void ShowTooltipWithTipAt(int x, int y);

  // Introspection
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

protected:
  // protected to simplify testing
  nux::ObjectPtr<StaticCairoText> _tooltip_text;

  void RecvCairoTextChanged(StaticCairoText* cairo_text);

  void PreLayoutManagement();

private:
  long PostLayoutManagement(long layoutResult);

  void PositionChildLayout(float offsetX,
                           float offsetY);

  void LayoutWindowElements();

  void NotifyConfigurationChange(int width,
                                 int height);

  int                   _anchorX;
  int                   _anchorY;

  nux::HLayout* _hlayout;
  nux::VLayout* _vlayout;
  nux::SpaceLayout* _left_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout* _right_space; //!< Space from the right of the text to the right of the widget.
  nux::SpaceLayout* _top_space;  //!< Space from the left of the widget to the left of the text.
  nux::SpaceLayout* _bottom_space; //!< Space from the right of the text to the right of the widget.

  bool _cairo_text_has_changed;
  void UpdateTexture();
};
}

#endif // TOOLTIP_H

