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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include "UnityWindowStyle.h"

using namespace unity::ui;

namespace
{

TEST(TestUnityWindowStyle, Get)
{
  auto const& style = UnityWindowStyle::Get();
  ASSERT_NE(style, nullptr);

  {
  auto style_copy = UnityWindowStyle::Get();
  ASSERT_EQ(style, style_copy);

  EXPECT_EQ(style.use_count(), 2);
  }

  EXPECT_EQ(style.use_count(), 1);
}

}