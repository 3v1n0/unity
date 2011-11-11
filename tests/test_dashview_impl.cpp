/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Marco Biscaro <marcobiscaro2112@gmail.com>
 */

#include <gmock/gmock.h>

#include "DashViewPrivate.h"

using namespace testing;

using namespace unity::dash::impl;

namespace
{

TEST(TestParseLensFilter, TestSimpleString)
{
  LensFilter filter; // = parse_lens_uri("simple");

  EXPECT_THAT(filter.id, Eq("simple"));
  EXPECT_TRUE(filter.filters.empty());
}

TEST(TestParseLensFilter, TestSingleParameter)
{
  LensFilter filter; // = parse_lens_uri("uri?param=test");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_FALSE(filter.filters.empty());
  EXPECT_THAT(filter.filters["param"], Eq("test"));
  EXPECT_THAT(filter.filters["unknown"], Eq(""));
}

TEST(TestParseLensFilter, TestMultipleParameters)
{
  LensFilter filter;// = parse_lens_uri("uri?param1=first&param2=second");

  EXPECT_THAT(filter.id, Eq("uri"));
  EXPECT_FALSE(filter.filters.empty());
  EXPECT_THAT(filter.filters["param1"], Eq("first"));
  EXPECT_THAT(filter.filters["param2"], Eq("second"));
  EXPECT_THAT(filter.filters["unknown"], Eq(""));
}

}
