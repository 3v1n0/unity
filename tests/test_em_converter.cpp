// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <gmock/gmock.h>
using namespace testing;

#include "unity-shared/EMConverter.h"

namespace unity
{

int const PIXEL_SIZE = 24;
int const FONT_SIZE  = 13;
double const DPI     = 96.0f;

class TestEMConverter : public Test
{
public:
  TestEMConverter()
    : em_converter(FONT_SIZE, DPI)
  {
  }

  EMConverter em_converter;
};

TEST_F(TestEMConverter, TestCtor)
{
  EXPECT_EQ(FONT_SIZE, em_converter.GetFontSize());
  EXPECT_EQ(DPI, em_converter.GetDPI());
}

TEST_F(TestEMConverter, TestSetFontSize)
{
  int const font_size = 15;

  em_converter.SetFontSize(font_size);
  EXPECT_EQ(font_size, em_converter.GetFontSize());
}

TEST_F(TestEMConverter, TestSetDPI)
{
  int const dpi = 120.0f;

  em_converter.SetDPI(dpi);
  EXPECT_EQ(dpi, em_converter.GetDPI());
}

TEST_F(TestEMConverter, TestEMToPixelFromPixelToEMGetsSamePixelSize)
{
  EXPECT_EQ(PIXEL_SIZE, em_converter.EMToPixels(em_converter.PixelsToEM(PIXEL_SIZE)));
}

} // namespace unity
