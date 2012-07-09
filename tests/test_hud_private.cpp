/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <gtest/gtest.h>

#include "hud/HudPrivate.h"
using namespace unity::hud;

namespace
{

TEST(TestHudPrivate, RefactorTextEmpty)
{
  std::vector<std::pair<std::string, bool>> temp;

  temp = impl::RefactorText("");
  ASSERT_EQ(temp.size(), 0);

  temp = impl::RefactorText("Test");
  ASSERT_EQ(temp.size(), 1);
  EXPECT_EQ(temp[0].first, "Test");
  EXPECT_EQ(temp[0].second, false); // True means "Full opacity", false "Half opacity"

  temp = impl::RefactorText("<b>Test</b>");
  ASSERT_EQ(temp.size(), 1);
  EXPECT_EQ(temp[0].first, "Test");
  EXPECT_EQ(temp[0].second, true);

  temp = impl::RefactorText("Hello > <b>Test</b> World");
  ASSERT_EQ(temp.size(), 3);
  EXPECT_EQ(temp[0].first, "Hello > ");
  EXPECT_EQ(temp[0].second, false);
  EXPECT_EQ(temp[1].first, "Test");
  EXPECT_EQ(temp[1].second, true);
  EXPECT_EQ(temp[2].first, " World");
  EXPECT_EQ(temp[2].second, false);

  temp = impl::RefactorText("Open <b>Fi</b>le <b>Wit</b>h");
  ASSERT_EQ(temp.size(), 5);
  EXPECT_EQ(temp[0].first, "Open ");
  EXPECT_EQ(temp[0].second, false);
  EXPECT_EQ(temp[1].first, "Fi");
  EXPECT_EQ(temp[1].second, true);
  EXPECT_EQ(temp[2].first, "le ");
  EXPECT_EQ(temp[2].second, false);
  EXPECT_EQ(temp[3].first, "Wit");
  EXPECT_EQ(temp[3].second, true);
  EXPECT_EQ(temp[4].first, "h");
  EXPECT_EQ(temp[4].second, false);

  temp = impl::RefactorText("Open <b>File With");
  ASSERT_EQ(temp.size(), 2);
  EXPECT_EQ(temp[0].first, "Open ");
  EXPECT_EQ(temp[0].second, false);
  EXPECT_EQ(temp[1].first, "File With");
  EXPECT_EQ(temp[1].second, true);
}

}
