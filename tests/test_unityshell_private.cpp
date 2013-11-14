/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <gtest/gtest.h>
 
#include "UnityshellPrivate.h"
#include "DeltaRestrainment-Inl.h"

using namespace unity;

namespace
{
const unsigned int RectangleWidth = 100;
const unsigned int RectangleHeight = 200;
const unsigned int RectangleX = 50;
const unsigned int RectangleY = 25;

const nux::Geometry Rectangle =
{
  RectangleX,
  RectangleY,
  RectangleWidth,
  RectangleHeight
};

const nux::Point2D<int> PointTL (RectangleX, RectangleY);
const nux::Point2D<int> PointBR (RectangleX + RectangleWidth,
                                 RectangleY + RectangleHeight);

}

TEST(DeltaRestrainment, RestrainOutWidth)
{
  int x = 1;
  int y = 0;

  util::restrainDelta(x, y, Rectangle, PointBR);

  EXPECT_EQ (x, 0);
  EXPECT_EQ (y, 0);
}

TEST(DeltaRestrainment, RestrainOutHeight)
{
  int x = 0;
  int y = 1;

  util::restrainDelta(x, y, Rectangle, PointBR);

  EXPECT_EQ (x, 0);
  EXPECT_EQ (y, 0);
}

TEST(DeltaRestrainment, RestrainOutX)
{
  int x = -1;
  int y = 0;

  util::restrainDelta(x, y, Rectangle, PointTL);

  EXPECT_EQ (x, 0);
  EXPECT_EQ (y, 0);
}

TEST(DeltaRestrainment, RestrainOutY)
{
  int x = 0;
  int y = -1;

  util::restrainDelta(x, y, Rectangle, PointTL);

  EXPECT_EQ (x, 0);
  EXPECT_EQ (y, 0);
}

TEST(TestUnityshellPrivate, TestCreateActionString)
{
  EXPECT_EQ(impl::CreateActionString("<Super>", 'a'), "<Super>a");
  EXPECT_EQ(impl::CreateActionString("<Super>", '1'), "<Super>1");

  EXPECT_EQ(impl::CreateActionString("<Alt>", 'a'), "<Alt>a");
  EXPECT_EQ(impl::CreateActionString("<Alt>", '1'), "<Alt>1");

  EXPECT_EQ(impl::CreateActionString("<Super>", '1', impl::ActionModifiers::USE_NUMPAD), "<Super>KP_1");

  EXPECT_EQ(impl::CreateActionString("<Super>", '1', impl::ActionModifiers::USE_SHIFT), "<Super><Shift>1");

  EXPECT_EQ(impl::CreateActionString("<Super>", '1', impl::ActionModifiers::USE_SHIFT_NUMPAD), "<Super><Shift>KP_1");
}

