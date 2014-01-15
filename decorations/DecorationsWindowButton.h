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

#ifndef UNITY_DECORATION_WINDOW_BUTTON
#define UNITY_DECORATION_WINDOW_BUTTON

#include "DecorationStyle.h"
#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class WindowButton : public TexturedItem
{
public:
  WindowButton(CompWindow*, WindowButtonType type);

  WidgetState GetCurrentState() const;

protected:
  void ButtonDownEvent(CompPoint const&, unsigned button);
  void ButtonUpEvent(CompPoint const&, unsigned button);
  void MotionEvent(CompPoint const&);

  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void UpdateTexture();

  WindowButtonType type_;
  bool pressed_;
  bool was_pressed_;
  CompWindow* win_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATION_WINDOW_BUTTON
