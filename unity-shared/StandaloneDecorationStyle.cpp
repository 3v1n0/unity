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

#include <iostream>
#include <gtk/gtk.h>
#include "unity-shared/DecorationStyle.h"

using namespace unity::decoration;

std::ostream& operator<<(std::ostream &os, Alignment const& a)
{
  switch (a)
  {
    case Alignment::LEFT:
      return os << "Left";
    case Alignment::CENTER:
      return os << "Center";
    case Alignment::RIGHT:
      return os << "Right";
    case Alignment::FLOATING:
      return os << "Floating at " + std::to_string(Style::Get()->TitleAlignmentValue());
  }

  return os;
}

std::ostream& operator<<(std::ostream &os, Border const& b)
{
  return os << "top " << b.top << ", left " << b.left << ", right " << b.right << ", bottom " << b.bottom;
}

int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);
  auto const& style = Style::Get();

  std::cout << "Border top: " << style->Border().top << std::endl;
  std::cout << "Border left: " << style->Border().left << std::endl;
  std::cout << "Border right: " << style->Border().right << std::endl;
  std::cout << "Border bottom: " << style->Border().bottom << std::endl;
  std::cout << "Input border top: " << style->InputBorder().top << std::endl;
  std::cout << "Input border left: " << style->InputBorder().left << std::endl;
  std::cout << "Input border right: " << style->InputBorder().right << std::endl;
  std::cout << "Input border bottom: " << style->InputBorder().bottom << std::endl;
  std::cout << "---" << std::endl;
  std::cout << "Title alignment: " << style->TitleAlignment() << std::endl;
  std::cout << "---" << std::endl;
  std::cout << "Top border padding, top: " << style->Padding(Side::TOP) << std::endl;
  std::cout << "Left border padding, left: " << style->Padding(Side::LEFT) << std::endl;
  std::cout << "Right border padding, right: " << style->Padding(Side::RIGHT) << std::endl;
  std::cout << "Bottom border padding, bottom: " << style->Padding(Side::BOTTOM) << std::endl;
  std::cout << "---" << std::endl;

  std::cout << "Button 'close' state 'normal' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::NORMAL) << std::endl;
  std::cout << "Button 'close' state 'prelight' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::PRELIGHT) << std::endl;
  std::cout << "Button 'close' state 'pressed' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::PRESSED) << std::endl;
  std::cout << "Button 'close' state 'disabled' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::DISABLED) << std::endl;
  std::cout << "Button 'close' state 'backdrop' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::BACKDROP) << std::endl;
  std::cout << "Button 'close' state 'backdrop_prelight' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::BACKDROP_PRELIGHT) << std::endl;
  std::cout << "Button 'close' state 'backdrop_pressed' " << style->WindowButtonFile(WindowButtonType::CLOSE, WidgetState::BACKDROP_PRESSED) << std::endl;
  std::cout << "Button 'minimize' state 'normal' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::NORMAL) << std::endl;
  std::cout << "Button 'minimize' state 'prelight' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::PRELIGHT) << std::endl;
  std::cout << "Button 'minimize' state 'pressed' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::PRESSED) << std::endl;
  std::cout << "Button 'minimize' state 'disabled' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::DISABLED) << std::endl;
  std::cout << "Button 'minimize' state 'backdrop' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::BACKDROP) << std::endl;
  std::cout << "Button 'minimize' state 'backdrop_prelight' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::BACKDROP_PRELIGHT) << std::endl;
  std::cout << "Button 'minimize' state 'backdrop_pressed' " << style->WindowButtonFile(WindowButtonType::MINIMIZE, WidgetState::BACKDROP_PRESSED) << std::endl;
  std::cout << "Button 'unmaximize' state 'normal' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE,WidgetState::NORMAL) << std::endl;
  std::cout << "Button 'unmaximize' state 'prelight' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::PRELIGHT) << std::endl;
  std::cout << "Button 'unmaximize' state 'pressed' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::PRESSED) << std::endl;
  std::cout << "Button 'unmaximize' state 'disabled' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::DISABLED) << std::endl;
  std::cout << "Button 'unmaximize' state 'backdrop' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::BACKDROP) << std::endl;
  std::cout << "Button 'unmaximize' state 'backdrop_prelight' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::BACKDROP_PRELIGHT) << std::endl;
  std::cout << "Button 'unmaximize' state 'backdrop_pressed' " << style->WindowButtonFile(WindowButtonType::UNMAXIMIZE, WidgetState::BACKDROP_PRESSED) << std::endl;
  std::cout << "Button 'maximize' state 'normal' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::NORMAL) << std::endl;
  std::cout << "Button 'maximize' state 'prelight' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::PRELIGHT) << std::endl;
  std::cout << "Button 'maximize' state 'pressed' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::PRESSED) << std::endl;
  std::cout << "Button 'maximize' state 'disabled' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::DISABLED) << std::endl;
  std::cout << "Button 'maximize' state 'backdrop' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::BACKDROP) << std::endl;
  std::cout << "Button 'maximize' state 'backdrop_prelight' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::BACKDROP_PRELIGHT) << std::endl;
  std::cout << "Button 'maximize' state 'backdrop_pressed' " << style->WindowButtonFile(WindowButtonType::MAXIMIZE, WidgetState::BACKDROP_PRESSED) << std::endl;
}
