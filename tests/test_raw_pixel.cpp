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

#include "unity-shared/RawPixel.h"

namespace unity
{

int const FONT_SIZE  = 13;
double const DPI     = 96.0;

class TestRawPixel : public Test
{
public:
  TestRawPixel()
    : cv(FONT_SIZE, DPI)
    , p_i(10_em)
    , p_f(10.0_em)
  {
  }

  EMConverter cv;
  RawPixel p_i;
  RawPixel p_f;
};

TEST_F(TestRawPixel, TestDefinedLiteralInt)
{
  ASSERT_EQ(p_i, 10);
}

TEST_F(TestRawPixel, TestDefinedLiteralFloat)
{
  ASSERT_EQ(p_f, 10.0);
}

TEST_F(TestRawPixel, TestCopy)
{
  RawPixel q = p_i;

  ASSERT_EQ(q, 10);
}

TEST_F(TestRawPixel, TestConverter)
{
  ASSERT_EQ(p_i.CP(cv), 10);
}

TEST_F(TestRawPixel, TestConverterTimesTwo)
{
  cv.SetDPI(DPI * 2);
  ASSERT_EQ(p_i.CP(cv), 20);
}

} // namespace unity
