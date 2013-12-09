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

std::string AlignementString(Style::Ptr const& style)
{
  switch (style->TitleAlignment())
  {
    case Alignment::LEFT:
      return "Left";
    case Alignment::CENTER:
      return "Center";
    case Alignment::RIGHT:
      return "Right";
    case Alignment::FLOATING:
      return "Floating at " + std::to_string(style->TitleAlignmentValue());
  }

  return "";
}

int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);
  auto const& style = Style::Get();

  std::cout << "Border top: " << style->BorderWidth(Side::TOP) << std::endl;
  std::cout << "Border left: " << style->BorderWidth(Side::LEFT) << std::endl;
  std::cout << "Border right: " << style->BorderWidth(Side::RIGHT) << std::endl;
  std::cout << "Border bottom: " << style->BorderWidth(Side::BOTTOM) << std::endl;
  std::cout << "---" << std::endl;
  std::cout << "Title alignment: " << AlignementString(style) << std::endl;
}
