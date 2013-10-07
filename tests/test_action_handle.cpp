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
#include <UnityCore/ConnectionManager.h>
#include <glib.h>

using namespace unity;

namespace
{

TEST(TestActionHandle, Initialization)
{
  action::handle handle;
  EXPECT_EQ(handle, 0);

  uint64_t val = g_random_int();
  action::handle random = val;
  EXPECT_EQ(random, val);
}

TEST(TestActionHandle, Assignment)
{
  action::handle handle;
  ASSERT_EQ(handle, 0);

  uint64_t val = g_random_int();
  handle = val;
  EXPECT_EQ(handle, val);
}

TEST(TestActionHandle, CastToScalarType)
{
  action::handle handle = 5;
  ASSERT_EQ(handle, 5);

  int int_handle = handle;
  EXPECT_EQ(int_handle, 5);

  unsigned uint_handle = handle;
  EXPECT_EQ(uint_handle, 5);
}

TEST(TestActionHandle, PrefixIncrementOperator)
{
  action::handle handle;
  ASSERT_EQ(handle, 0);

  for (auto i = 1; i <= 10; ++i)
  {
    ASSERT_EQ(++handle, i);
    ASSERT_EQ(handle, i);
  }
}

TEST(TestActionHandle, PostfixIncrementOperator)
{
  action::handle handle;
  ASSERT_EQ(handle, 0);

  for (auto i = 1; i <= 10; ++i)
  {
    ASSERT_EQ(handle++, i-1);
    ASSERT_EQ(handle, i);
  }
}

TEST(TestActionHandle, PrefixDecrementOperator)
{
  action::handle handle(10);
  ASSERT_EQ(handle, 10);

  for (auto i = 10; i > 0; --i)
  {
    ASSERT_EQ(--handle, i-1);
    ASSERT_EQ(handle, i-1);
  }
}

TEST(TestActionHandle, PostfixDecrementOperator)
{
  action::handle handle(10);
  ASSERT_EQ(handle, 10);

  for (auto i = 10; i > 0; --i)
  {
    ASSERT_EQ(handle--, i);
    ASSERT_EQ(handle, i-1);
  }
}

} // Namespace
