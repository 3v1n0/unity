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
#include "StaticCairoText.h"
#include "Introspectable.h"

namespace unity
{
class Tooltip : public CairoBaseWindow, public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(Tooltip, CairoBaseWindow);
public:
  Tooltip();

  void Draw(nux::GraphicsEngine& gfxContext, bool forceDraw);
  void DrawContent(nux::GraphicsEngine& gfxContext, bool forceDraw);

  void SetText(std::string const& text);

  void ShowTooltipWithTipAt(int anchor_tip_x, int anchor_tip_y);

  // Introspection
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

private:
  void RecvCairoTextChanged(nux::StaticCairoText* cairo_text);

  void PreLayoutManagement();

  long PostLayoutManagement(long layoutResult);

  void PositionChildLayout(float offsetX,
                           float offsetY);

  void LayoutWindowElements();

  void NotifyConfigurationChange(int width,
                                 int height);

  int                   _anchorX;
  int                   _anchorY;
  std::string           _labelText;

  nux::ObjectPtr<nux::StaticCairoText> _tooltip_text;

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

